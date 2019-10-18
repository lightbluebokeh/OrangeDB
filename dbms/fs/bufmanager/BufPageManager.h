#pragma once

#include "FindReplace.h"
#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"
#include <mutex>
#include <cassert>
#include <stddef.h>
#include <unordered_map>
#include <record/bytes_io.h>
#include <fs/bufmanager/buf_page_map.h>

class BufPage;

class BufPageManager {
    FileManager* fileManager;
    BufPageMap buf_page_map;
    FindReplace replace;
    bool dirty[BUF_CAP];
    bytes_t addr[BUF_CAP];

    bytes_t alloc_page() { return new byte_t[PAGE_SIZE]; }
    
    // 大概是缓存页面的函数
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

    BufPageManager(FileManager* fm) : replace(BUF_CAP), fileManager(fm) {
        memset(dirty, 0, sizeof(dirty));
        memset(addr, 0, sizeof(addr));
    }

    BufPageManager(const BufPageManager&) = delete;

    buf_t get_page_buf(page_t page) {
        int buf_id = buf_page_map.get_buf_id(page);
        if (buf_id != -1) {
            access(buf_id);
            return {addr[buf_id], buf_id};
        } else {
            auto buf = fetch_page(page);
            fileManager->read_page(page, buf.bytes, 0);
            return buf;
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
    page_t get_page(int buf_id) { return buf_page_map.get_page(buf_id);  }

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

    friend class BufPage;
};
