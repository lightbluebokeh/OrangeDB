#pragma once

#include "FindReplace.h"
#include "fs/fileio/FileManager.h"
#include "fs/utils/MyBitMap.h"
#include "fs/utils/pagedef.h"
#include <mutex>
#include <cassert>
#include <stddef.h>
#include <unordered_map>
#include <record/bytes_io.h>
#include <fs/bufmanager/buf_page_map.h>

class BufPage;

/*
 * BufPageManager
 * 实现了一个缓存的管理器
 */
class BufPageManager {
    int last;
    FileManager* fileManager;
    // Hash* hash;
    BufPageMap buf_page_map;
    // FindReplace* replace;
    FindReplace replace;
    bool dirty[BUF_CAP];
    /*
     * 缓存页面数组
     */
    bytes_t addr[BUF_CAP];
    // Page *pages;

    bytes_t alloc_page() { return new byte_t[PAGE_SIZE]; }
    buf_t fetch_page(page_t page) {
        bytes_t b;
        int buf_id = replace.find();
        b = addr[buf_id];
        if (b == nullptr) {
            b = alloc_page();
            addr[buf_id] = b;
        } else if (dirty[buf_id]) {
            // auto page = b->getKeys(buf_id, k1, k2);
            auto page = buf_page_map.get_page(buf_id);
            fileManager->write_page(page, b);
            dirty[buf_id] = false;
        }
        // hash->replace(buf_id, fileID, pageID);
        buf_page_map.replace(buf_id, page);
        // return b;
        return {b, buf_id};
    }

    /*
     * 构造函数
     * @参数fm:文件管理器，缓存管理器需要利用文件管理器与磁盘进行交互
     */
    BufPageManager(FileManager* fm) : replace(BUF_CAP), fileManager(fm) {
        last = -1;
        memset(dirty, 0, sizeof(dirty));
        memset(addr, 0, sizeof(addr));
        // for (int i = 0; i < BUF_CAP; ++i) {
        //     dirty[i] = false;
        //     addr[i] = nullptr;
        // }
    }

    BufPageManager(const BufPageManager&) = delete;

    /*
     * @函数名getPage
     * @参数fileID:文件id
     * @参数pageID:文件页号
     * @参数index:函数返回时，用来记录缓存页面数组中的下标
     * 返回:缓存页面的首地址
     * 功能:为文件中的某一个页面在缓存中找到对应的缓存页面
     *           文件页面由(fileID,pageID)指定
     *           缓存中的页面在缓存页面数组中的下标记录在index中
     *           首先，在hash表中查找(fileID,pageID)对应的缓存页面，
     *           如果能找到，那么表示文件页面在缓存中
     *           如果没有找到，那么就利用替换算法获取一个页面，第二个为页在缓存数组中的下标
     */
    buf_t get_page_buf(page_t page) {
        // int buf_id = hash->findIndex(fileID, pageID);
        int buf_id = buf_page_map.get_buf_id(page);
        if (buf_id != -1) {
            access(buf_id);
            return {addr[buf_id], buf_id};
        } else {
            // bytes_t b = fetch_page(fileID, pageID, buf_id);
            auto buf = fetch_page(page);
            fileManager->read_page(page, buf.bytes, 0);
            return buf;
            // return std::make_pair(b, buf_id);
            // return {b, buf_id};
        }
    }

    /*
     * @函数名markDirty
     * @参数index:缓存页面数组中的下标，用来表示一个缓存页面
     * 功能:标记index代表的缓存页面被写过，保证替换算法在执行时能进行必要的写回操作，
     *           保证数据的正确性
     */
    void mark_dirty(int buf_id) {
        dirty[buf_id] = true;
        access(buf_id);
    }

    // /*
    //  * @函数名allocPage
    //  * @参数fileID:文件id，数据库程序在运行时，用文件id来区分正在打开的不同的文件
    //  * @参数pageID:文件页号，表示在fileID指定的文件中，第几个文件页
    //  * @参数index:函数返回时，用来记录缓存页面数组中的下标
    //  * @参数ifRead:是否要将文件页中的内容读到缓存中
    //  * 返回:缓存页面的首地址
    //  * 功能:为文件中的某一个页面获取一个缓存中的页面
    //  *           缓存中的页面在缓存页面数组中的下标记录在index中
    //  *           并根据ifRead是否为true决定是否将文件中的内容写到获取的缓存页面中
    //  * 注意:在调用函数allocPage之前，调用者必须确信(fileID,pageID)指定的文件页面不存在缓存中
    //  *           如果确信指定的文件页面不在缓存中，那么就不用在hash表中进行查找，直接调用替换算法，节省时间
    //  */
    // bytes_t allocPage(int fileID, int pageID, int& buf_id, bool ifRead = false) {
    //     bytes_t b = fetch_page(fileID, pageID, buf_id);
    //     if (ifRead) {
    //         fileManager->readPage(fileID, pageID, b, 0);
    //     }
    //     return b;
    // }

    /*
     * @函数名access
     * @参数index:缓存页面数组中的下标，用来表示一个缓存页面
     * 功能:标记index代表的缓存页面被访问过，为替换算法提供信息
     */
    void access(int buf_id) {
        if (buf_id == last) {
            return;
        }
        replace.access(buf_id);
        last = buf_id;
    }

    /*
     * @函数名write_back
     * @参数index:缓存页面数组中的下标，用来表示一个缓存页面
     * 功能:将index代表的缓存页面归还给缓存管理器，在归还前，缓存页面中的数据需要根据脏页标记决定是否写到对应的文件页面中
     */
    void write_back(int buf_id) {
        if (dirty[buf_id]) {
            int f, p;
            // hash->getKeys(buf_id, f, p);
            auto page = buf_page_map.get_page(buf_id);
            fileManager->write_page(page, addr[buf_id]);
            dirty[buf_id] = false;
        }
        // replace->free(buf_id);
        replace.free(buf_id);
        buf_page_map.remove_buf(buf_id);
        // hash->remove(buf_id);
    }
public:
    // /*
    //  * @函数名release
    //  * @参数index:缓存页面数组中的下标，用来表示一个缓存页面
    //  * 功能:将index代表的缓存页面归还给缓存管理器，在归还前，缓存页面中的数据不标记写回，discard changes
    //  */
    // void release(int buf_id) {
    //     dirty[buf_id] = false;
    //     // replace->free(buf_id);
    //     replace.
    //     hash->remove(buf_id);
    // }

    /*
     * @函数名close
     * 功能:将所有缓存页面归还给缓存管理器，归还前需要根据脏页标记决定是否写到对应的文件页面中
     */
    void close() {
        for (int i = 0; i < BUF_CAP; ++i) {
            write_back(i);
        }
    }

    // /*
    //  * @函数名getKey
    //  * @参数index:缓存页面数组中的下标，用来指定一个缓存页面
    //  * @参数fileID:函数返回时，用于存储指定缓存页面所属的文件号
    //  * @参数pageID:函数返回时，用于存储指定缓存页面对应的文件页号
    //  */
    // void getKey(int buf_id, int& fileID, int& pageID) { hash->getKeys(buf_id, fileID, pageID); }
    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id) {
        access(buf_id);
        return buf_page_map.get_page(buf_id);
    }

    static BufPageManager* get_instance() {
        static std::mutex mtx;
        static BufPageManager *instance = nullptr;
        if (instance == nullptr) {
            mtx.lock();
            if (instance == nullptr) {
                instance = new BufPageManager(FileManager::get_instance());
            }
            mtx.unlock();
        }
        return instance;
    }

    // template<typename T>
    // std::enable_if_t<is_byte_v<T>, size_t> write_obj(int file_id, int page_id, int offset, const std::vector<T>& vec, int n) {
    //     assert(offset + n <= PAGE_SIZE);
    //     auto [buf, id] = get_page_buf(file_id, page_id);
    //     size_t ret = BytesIO::write_obj(buf, vec, n);
    //     mark_dirty(id);
    //     return ret;
    // }

    // template<typename T>
    // std::enable_if_t<is_byte_v<T>, size_t> write_bytes(int file_id, int page_id, int offset, const std::basic_string<T>& str, int n) {
    //     assert(offset + n <= PAGE_SIZE);
    //     auto [buf, id] = get_page_buf(file_id, page_id);
    //     size_t ret = BytesIO::write_bytes(buf, str, n);
    //     mark_dirty(id);
    //     return ret;        
    // }

    // template<typename T>
    // size_t write_bytes(int file_id, int page_id, int offset, const T& t, size_t n = sizeof(T)) {
    //     assert(offset + n <= PAGE_SIZE);
    //     auto [buf, id] = get_page_buf(file_id, page_id);
    //     size_t ret = BytesIO::write_bytes(buf + offset, t, n);
    //     mark_dirty(id);
    //     return ret;
    // }

    friend class BufPage;
};
