#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include <defs.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_manage.h>
#include <fs/bufpage/bufpage_stream.h>
#include <fs/file/file_manage.h>
#include "orange/common.h"

class Column;
class f_key_t;

// 打开的文件
class File {
private:
    int id;
    String name;
    size_t offset;

    File(int id, const String& name) : id(id), name(name) {}
    File(const File&) = delete;

    ~File() {}

    Bufpage get_bufpage(int page_id) { return Bufpage(id, page_id); }

    static File* files[MAX_FILE_NUM];

public:
    static bool create(const String& name) {
        orange_ensure(FileManage::create_file(name.c_str()) == 0, "file create fail");
        return true;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        int id, fd;
        orange_ensure(FileManage::open_file(name.c_str(), id, fd) == 0, "file open failed");
        if (files[id] == nullptr) files[id] = new File(id, name);
        return files[id];
    }

    static File* create_open(const String& name) {
        create(name);
        return open(name);
    }

    bool close() {
        orange_ensure(FileManage::close_file(id) == 0, "close file fail");
        orange_ensure(this == files[id], "this is magic");
        files[id] = nullptr;
        delete this;
        return true;
    }

    static bool remove(const String& name) {
        // 偷懒.jpg
        // if (FileManage::file_opened(name)) open(name)->close();
        orange_ensure(FileManage::remove_file(name) == 0, "remove file failed");
        return true;
    }

    File* write_bytes(const_bytes_t bytes, size_t n) {
        int page_id = int(offset >> PAGE_SIZE_IDX);
        BufpageStream bps(Bufpage(id, page_id));
        bps.seekpos(offset & (PAGE_SIZE - 1));
        auto rest = bps.rest(), tot = n;
        if (rest >= tot) {
            bps.write_bytes(bytes, tot);
        } else {
            bps.write_bytes(bytes, bps.rest());
            bytes += rest;
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.write_bytes(bytes, PAGE_SIZE);
                bytes += PAGE_SIZE;
                tot -= PAGE_SIZE;
                page_id++;
            }
            if (tot) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.write_bytes(bytes, tot);
            }
        }
        offset += tot;
        return this;
    }

    template <typename T, typename... Ts>
    File* write(const T& t, const Ts&... ts) {
        if constexpr (is_std_vector_v<T> || is_basic_string_v<T>) {
            write(t.size());
            for (auto x : t) write(x);
        } else if constexpr (std::is_same_v<T, pred_t>) {
            write(t.lo, t.lo_eq, t.hi, t.hi_eq);
        } else if constexpr (std::is_same_v<T, Column>) {
            // write(t.name, t.type, t.maxsize, t.p, t.s, t.unique, t.nullable, t.index, t.dft, t.ranges);
            ORANGE_UNREACHABLE
        } else if constexpr (std::is_same_v<T, f_key_t>) {
            write(t.name, t.ref_tbl, t.list, t.ref_list);
        } else {
            write_bytes((bytes_t)&t, sizeof(T));
        }
        if constexpr (sizeof...(Ts) != 0) write(ts...);
        return this;
    }

    File* read_bytes(bytes_t bytes, size_t n) {
        int page_id = int(offset >> PAGE_SIZE_IDX);
        BufpageStream bps(Bufpage(id, page_id));
        bps.seekpos(offset & (PAGE_SIZE - 1));
        auto rest = bps.rest(), tot = n;
        if (rest >= tot) {
            bps.read_bytes(bytes, tot);
        } else {
            bps.read_bytes(bytes, bps.rest());
            bytes += rest;
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.read_bytes(bytes, PAGE_SIZE);
                bytes += PAGE_SIZE;
                tot -= PAGE_SIZE;
                page_id++;
            }
            if (tot) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.read_bytes(bytes, tot);
            }
        }
        offset += n;
        return this;
    }

    template <typename T, typename... Ts>
    File* read(T& t, Ts&... ts) {
        if constexpr (is_std_vector_v<T> || is_basic_string_v<T>) {
            size_t size;
            read(size);
            t.resize(size);
            for (auto& x : t) read(x);
        } else if constexpr (std::is_same_v<T, pred_t>) {
            read(t.lo, t.lo_eq, t.hi, t.hi_eq);
        } else if constexpr (std::is_same_v<T, Column>) {
            ORANGE_UNREACHABLE
            // read(t.name, t.type, t.maxsize, t.p, t.s, t.unique, t.nullable, t.index, t.dft, t.ranges);
        } else if constexpr (std::is_same_v<T, f_key_t>) {
            read(t.name, t.ref_tbl, t.list, t.ref_list);
        } else {
            read_bytes((bytes_t)&t, sizeof(T));
        }
        if constexpr (sizeof...(Ts) != 0) read(ts...);
        return this;
    }

    template <typename T>
    T read() {
        T t;
        read(t);
        return t;
    }

    File* seek_pos(size_t pos) {
        offset = pos;
        return this;
    }
};
