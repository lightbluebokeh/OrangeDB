#pragma once

#include <mutex>
#include <string>
#include <cassert>

#include <fs/file/file_manager.h>
#include <fs/bufpage/bufpage_manager.h>
#include <utils/YourLinkList.h>
#include <utils/find_replace.h>
#include <fs/bufpage/bufpage_map.h>


class Bufpage;
class BufpageStream;

class BufpageManager {
    FileManager* fileManager;
    FindReplace replace;
    BufpageMap bufpage_map;
    bool dirty[BUF_CAP];
    bytes_t addr[BUF_CAP];

    bytes_t alloc_page() { return new byte_t[PAGE_SIZE]; }

    // 为 page 分配新的缓存编号
    buf_t fetch_page(page_t page) {
        bytes_t b;
        int buf_id = replace.find();
        b = addr[buf_id];
        if (b == nullptr) {
            b = alloc_page();
            addr[buf_id] = b;
        } else {
            write_back(buf_id);
        }

        bufpage_map.set_map(page, buf_id);
        return {b, buf_id};
    }

    BufpageManager(FileManager* fm) : fileManager(fm), replace() {
        memset(dirty, 0, sizeof(dirty));
        memset(addr, 0, sizeof(addr));
    }

    BufpageManager(const BufpageManager&) = delete;

    buf_t get_page_buf(page_t page) {
        int buf_id = bufpage_map.get_buf_id(page);
        if (buf_id == -1) {
            auto buf = fetch_page(page);
            fileManager->read_page(page, buf.bytes);
            return buf;
        } else {
            access(buf_id);
            return {addr[buf_id], buf_id};
        }
    }

    void mark_dirty(int buf_id) { dirty[buf_id] = true; }

    void access(int buf_id) {
        static int last = -1;
        if (buf_id == last) return;
        replace.access(buf_id);
        last = buf_id;
    }

    void write_back(int buf_id) {
        if (dirty[buf_id]) {
            auto page = bufpage_map.get_page(buf_id);
            fileManager->write_page(page, addr[buf_id]);
            dirty[buf_id] = false;
            access(buf_id);
        }
    }

public:
    void write_back() {
        for (int i = 0; i < BUF_CAP; ++i) {
            write_back(i);
        }
    }

    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id) { return bufpage_map.get_page(buf_id); }

    bool tracking(page_t page) { return bufpage_map.contains(page); }

    static BufpageManager* get_instance() {
        static std::mutex mtx;
        static BufpageManager* instance = nullptr;
        if (instance == nullptr) {
            mtx.lock();
            if (instance == nullptr) {
                instance = new BufpageManager(FileManager::get_instance());
            }
            mtx.unlock();
        }
        return instance;
    }

    void wirte_back_file(int file_id) {
        for (int buf_id: bufpage_map.get_all(file_id)) {
            write_back(buf_id);
        }
    }

    friend class Bufpage;
    friend class BufpageStream;
};
