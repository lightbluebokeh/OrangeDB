#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "defs.h"
#include "fs/bufpage/bufpage.h"
#include "fs/bufpage/bufpage_manage.h"
#include "fs/file/file_manage.h"
#include "orange/common.h"

class Column;
class f_key_t;

// 打开的文件
class File {
private:
    int id;
    String name;
    // size_t offset;

    File(int id, const String& name) : id(id), name(name) {}
    File(const File&) = delete;
    ~File() {}

    Bufpage get_bufpage(int page_id) const { 
        Bufpage page(id, page_id);
        page.ensure_buf();
        return page;
    }

    static File* files[MAX_FILE_NUM];

public:
    String get_name() const { return name; }

    static bool create(const String& name) {
        orange_assert(FileManage::create_file(name.c_str()) == 0, "file create fail");
        return true;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        int id, fd;
        orange_assert(FileManage::open_file(name.c_str(), id, fd) == 0, "file open failed: " + name);
        if (files[id] == nullptr) files[id] = new File(id, name);
        return files[id];
    }

    static File* create_open(const String& name) {
        create(name);
        return open(name);
    }

    bool close() {
        orange_assert(FileManage::close_file(id) == 0, "close file fail");
        orange_assert(this == files[id], "this is magic");
        files[id] = nullptr;
        delete this;
        return true;
    }

    static bool remove(const String& name) {
        orange_assert(FileManage::remove_file(name) == 0, "remove file failed");
        return true;
    }

    size_t write_bytes(size_t offset, const_bytes_t bytes, size_t n) const {
        int page_id = int(offset >> PAGE_SIZE_IDX);
        // BufpageStream bps(Bufpage(id, page_id));
        // Bufpage page(id, page_id);
        auto page = get_bufpage(page_id);
        // bps.seekpos(offset & (PAGE_SIZE - 1));
        auto pos = (offset & (PAGE_SIZE - 1));
        // auto rest = bps.rest(), tot = n;
        auto rest = PAGE_SIZE - pos, tot = n;
        if (rest >= tot) {
            // bps.write_bytes(bytes, tot);
            memcpy(page.buf.bytes + pos, bytes, tot);
            BufpageManage::mark_dirty(page.buf.buf_id);
        } else {
            // bps.write_bytes(bytes, bps.rest());
            memcpy(page.buf.bytes + pos, bytes, rest);
            BufpageManage::mark_dirty(page.buf.buf_id);
            bytes += rest;
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                // bps = BufpageStream(Bufpage(id, page_id));
                page = get_bufpage(page_id);
                // bps.write_bytes(bytes, PAGE_SIZE);
                memcpy(page.buf.bytes, bytes, PAGE_SIZE);
                BufpageManage::mark_dirty(page.buf.buf_id);
                bytes += PAGE_SIZE;
                tot -= PAGE_SIZE;
                page_id++;
            }
            if (tot) {
                // bps = BufpageStream(Bufpage(id, page_id));
                page = get_bufpage(page_id);
                // bps.write_bytes(bytes, tot);
                memcpy(page.buf.bytes, bytes, tot);
                BufpageManage::mark_dirty(page.buf.buf_id);
            }
        }
        return n;
        // offset += tot;
        // return this;
    }

    template <typename T, typename... Ts>
    size_t write(size_t offset, const T& t, const Ts&... ts) const {
        size_t ret = 0;
        if constexpr (is_std_vector_v<T> || is_basic_string_v<T>) {
            ret = write(offset, t.size());
            for (auto x : t) ret += write(offset + ret, x);
        } else {
            ret = write_bytes(offset, (bytes_t)&t, sizeof(T));
        }
        if constexpr (sizeof...(Ts) != 0) ret += write(offset + ret, ts...);
        return ret;
    }

    size_t read_bytes(size_t offset, bytes_t bytes, size_t n) const {
        int page_id = int(offset >> PAGE_SIZE_IDX);
        // BufpageStream bps(Bufpage(id, page_id));
        auto page = get_bufpage(page_id);
        // bps.seekpos(offset & (PAGE_SIZE - 1));
        size_t pos = offset & (PAGE_SIZE - 1);
        // auto rest = bps.rest(), tot = n;
        auto rest = PAGE_SIZE - pos, tot = n;
        if (rest >= tot) {
            // bps.read_bytes(bytes, tot);
            memcpy(bytes, page.buf.bytes + pos, tot);
        } else {
            // bps.read_bytes(bytes, bps.rest());
            memcpy(bytes, page.buf.bytes + pos, rest);
            bytes += rest;
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                // bps = BufpageStream(Bufpage(id, page_id));
                page = get_bufpage(page_id);
                // bps.read_bytes(bytes, PAGE_SIZE);
                memcpy(bytes, page.buf.bytes, PAGE_SIZE);
                bytes += PAGE_SIZE;
                tot -= PAGE_SIZE;
                page_id++;
            }
            if (tot) {
                // bps = BufpageStream(Bufpage(id, page_id));
                page = get_bufpage(page_id);
                // bps.read_bytes(bytes, tot);
                memcpy(bytes, page.buf.bytes, tot);
            }
        }
        return n;
        // offset += n;
        // return this;
    }

    template <typename T, typename... Ts>
    size_t read(size_t offset, T& t, Ts&... ts) const {
        size_t ret = 0;
        if constexpr (is_std_vector_v<T> || is_basic_string_v<T>) {
            size_t size;
            ret = read(offset, size);
            t.resize(size);
            for (auto& x : t) ret += read(offset + ret, x);
        } else {
            ret = read_bytes(offset + ret, (bytes_t)&t, sizeof(T));
        }
        if constexpr (sizeof...(Ts) != 0) ret += read(offset + ret, ts...);
        return ret;
    }

    template <typename T>
    T read(size_t offset) const {
        T t;
        read(offset, t);
        return t;
    }
};
