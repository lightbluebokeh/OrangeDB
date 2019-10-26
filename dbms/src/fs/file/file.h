#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include <defs.h>
#include <fs/file/file_manage.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_manage.h>
#include <fs/bufpage/bufpage_stream.h>

// 打开的文件
class File {
private:
    int id, fd;
    String name;
    size_t offset;

    File(int id, int fd, const String& name) : id(id), fd(fd), name(name) {}
    File(const File&) = delete;

    ~File() {}

    Bufpage get_bufpage(int page_id) { return Bufpage(id, page_id); }

    static File* files[MAX_FILE_NUM];
public:
    static bool create(const std::string& name) {
        ensure(FileManage::create_file(name.c_str()) == 0, "file create fail");
        return 1;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        int id, fd;
        ensure(FileManage::open_file(name.c_str(), id, fd) == 0, "file open failed");
        if (files[id] == nullptr) files[id] = new File(id, fd, name);
        return files[id];
    }

    bool close() {
        ensure(FileManage::close_file(id) == 0, "close file fail");
        ensure(this == files[id], "this is magic");
        files[id] = nullptr;
        delete this;
        return 1;
    }

    static bool remove(const String& name) {
        // 偷懒.jpg
        if (FileManage::file_opened(name)) open(name)->close();
        ensure(FileManage::remove_file(name) == 0, "remove file failed");
        return 1;
    }
 
    File* write_bytes(const std::remove_pointer_t<bytes_t>* bytes, size_t n) {
        int page_id = offset >> PAGE_SIZE_IDX;
        BufpageStream bps(Bufpage(id, page_id));
        bps.seekpos(offset & (PAGE_SIZE - 1));
        auto rest = bps.rest(), tot = n;
        if (rest >= tot) {
            bps.write_bytes(bytes, tot);
        } else {
            bps.write_bytes(bytes, bps.rest());
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.write_bytes(bytes, PAGE_SIZE);
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

    template<typename T, typename... Ts>
    File* write(const T& t, const Ts&... ts) {
        auto ret = this;
        if constexpr (is_std_vector_v<T>) {
            ret = ret->write(t.size());
            for (auto x: t) {
                ret = ret->write(x);
            }
        } else {
            ret = ret->write_bytes((bytes_t)&t, sizeof(T));
        }
        if constexpr (sizeof...(Ts) != 0) {
            ret = ret->template write(ts...);
        }
        return ret;
    }

    // warning: 直接写文件的时候没有将缓存写回，仅供测试
    File* read_bytes(bytes_t bytes, size_t n) {
        int page_id = offset >> PAGE_SIZE_IDX;
        BufpageStream bps(Bufpage(id, page_id));
        bps.seekpos(offset & (PAGE_SIZE - 1));
        auto rest = bps.rest(), tot = n;
        if (rest >= tot) {
            bps.read_bytes(bytes, tot);
        } else {
            bps.read_bytes(bytes, bps.rest());
            tot -= rest;
            page_id++;
            while (tot >= PAGE_SIZE) {
                bps = BufpageStream(Bufpage(id, page_id));
                bps.read_bytes(bytes, PAGE_SIZE);
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

    template<typename T, typename... Ts>
    File* read(T& t, Ts&... ts) {
        auto ret = this;
        if constexpr (is_std_vector_v<T>) {
            using size_t = typename T::size_type;
            size_t size;
            ret = ret->read(size);
            t.resize(size);
            for (size_t i = 0; i < size; i++) {
                ret = ret->read(t[i]);
            }
        } else {
            ret = ret->read_bytes((bytes_t)&t, sizeof(T)); 
        }
        if constexpr (sizeof...(Ts) != 0) {
            ret = ret->read(ts...);
        }
        return ret;
    }

    template<typename T>
    T read() {
        T t;
        read(t);
        return t;
    }

    File* seek_pos(size_t pos) { offset = pos; return this; }
    File* seek_off(size_t off) { offset += off; return this; }
};