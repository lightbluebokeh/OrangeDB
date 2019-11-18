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

class Column;

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
        ensure(FileManage::create_file(name.c_str()) == 0, "file create fail");
        return true;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        int id, fd;
        ensure(FileManage::open_file(name.c_str(), id, fd) == 0, "file open failed");
        if (files[id] == nullptr) files[id] = new File(id, name);
        return files[id];
    }

    static File* create_open(const String& name) {
        create(name);
        return open(name);
    }

    bool close() {
        ensure(FileManage::close_file(id) == 0, "close file fail");
        ensure(this == files[id], "this is magic");
        files[id] = nullptr;
        delete this;
        return true;
    }

    static bool remove(const String& name) {
        // 偷懒.jpg
        // if (FileManage::file_opened(name)) open(name)->close();
        ensure(FileManage::remove_file(name) == 0, "remove file failed");
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

    template <typename... T>
    File* write(const T&... args) {
        auto write_each = [&](auto&& arg) {
            using arg_t = decltype(arg);
            if constexpr (is_std_vector_v<arg_t> || is_basic_string_v<arg_t>) {
                write(arg.size());
                for (auto x : arg) write(x);
            } else if constexpr (std::is_same_v<arg_t, pred_t>) {
                write(arg.lo, arg.lo_eq, arg.hi, arg.hi_eq);
            } else if constexpr (std::is_same_v<arg_t, Column>) {
                write(arg.name, arg.kind, arg.maxsize, arg.p, arg.s, arg.unique, arg.nullable, arg.index, arg.dft, arg.ranges);
            } else {
                write_bytes((bytes_t)&arg, sizeof(arg_t));
            }
        };
        expand(write_each, args...);
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
            read(t.name, t.kind, t.maxsize, t.p, t.s, t.unique, t.nullable, t.index, t.dft, t.ranges);
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
    File* seek_off(size_t off) {
        offset += off;
        return this;
    }

    // // 这两个函数效率可能比较慢，而且没有考虑缓存，慎用
    // size_t size() { return fs::file_size(name); }
    // void resize(size_t size) { fs::resize_file(name, size); }
};