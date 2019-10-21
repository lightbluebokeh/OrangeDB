#pragma once

#include <cassert>

#include <bytes/bytes_io.h>
#include <fs/bufmanager/FindReplace.h>
#include <fs/bufmanager/buf_page_map.h>
#include <fs/fileio/FileManager.h>

#include <mutex>

class BufPage;
class BufPageStream;

class BufPageManager {
    FileManager* fileManager;
    FindReplace replace;
    BufPageMap buf_page_map;
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

        buf_page_map.set_map(page, buf_id);
        return {b, buf_id};
    }

    BufPageManager(FileManager* fm) : fileManager(fm), replace(BUF_CAP) {
        memset(dirty, 0, sizeof(dirty));
        memset(addr, 0, sizeof(addr));
    }

    BufPageManager(const BufPageManager&) = delete;

    buf_t get_page_buf(page_t page) {
        int buf_id = buf_page_map.get_buf_id(page);
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
            auto page = buf_page_map.get_page(buf_id);
            fileManager->write_page(page, addr[buf_id]);
            dirty[buf_id] = false;
        }
    }

public:
    void close() {
        for (int i = 0; i < BUF_CAP; ++i) {
            write_back(i);
        }
    }

    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id) { return buf_page_map.get_page(buf_id); }

    bool tracking(page_t page) { return buf_page_map.contains(page); }

    static BufPageManager* get_instance() {
        static std::mutex mtx;
        static BufPageManager* instance = nullptr;
        if (instance == nullptr) {
            mtx.lock();
            if (instance == nullptr) {
                instance = new BufPageManager(FileManager::get_instance());
            }
            mtx.unlock();
        }
        return instance;
    }

    friend class BufPage;
    friend class BufPageStream;
};
