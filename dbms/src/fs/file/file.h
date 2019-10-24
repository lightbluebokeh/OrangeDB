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
 
    template<bool use_buf = true>
    File* write_bytes(const std::remove_pointer_t<bytes_t>* bytes, size_t n) {
        if constexpr (use_buf == 0) {
            lseek(fd, offset, SEEK_SET);
            ::write(fd, bytes, n);
        } else {
            int page_id = offset >> PAGE_SIZE_IDX;
            auto offset  = this->offset & (PAGE_SIZE - 1);
            BufpageStream bps(Bufpage(id, page_id));
            bps.seekpos(offset);
            auto rest = bps.rest();
            if (rest >= n) {
                bps.write_bytes(bytes, n);
            } else {
                bps.write_bytes(bytes, bps.rest());
                n -= rest;
                page_id++;
                while (n >= PAGE_SIZE) {
                    bps = BufpageStream(Bufpage(id, page_id));
                    bps.write_bytes(bytes, PAGE_SIZE);
                    n -= PAGE_SIZE;
                    page_id++;
                }
                if (n) {
                    bps = BufpageStream(Bufpage(id, page_id));
                    bps.write_bytes(bytes, n);
                }
            }
        }
        offset += n;
        return this;
    }

    template<typename T, bool use_buf = true>
    File* write(const T& t, size_t n = sizeof(T)) {
        if constexpr (is_std_vector_v<T>) {
            File* ret = write<typename T::size_type, use_buf>(t.size());
            for (auto x: t) {
                ret->write<typename T::value_type, use_buf>(x);
            }
            return ret;
        } else {
            return write_bytes<use_buf>((bytes_t)&t, n);
        }
    }

    // warning: 直接写文件的时候没有将缓存写回，仅供测试
    template<bool use_buf = true>
    File* read_bytes(bytes_t bytes, size_t n) {
        if constexpr (use_buf == 0) {
            lseek(fd, offset, SEEK_SET);
            ::read(fd, bytes, n);
        } else {
            int page_id = offset >> PAGE_SIZE_IDX;
            auto offset = this->offset & (PAGE_SIZE - 1);
            // offset &= PAGE_SIZE - 1;
            BufpageStream bps(Bufpage(id, page_id));
            bps.seekpos(offset);
            auto rest = bps.rest();
            if (rest >= n) {
                bps.read_bytes(bytes, n);
            } else {
                bps.read_bytes(bytes, bps.rest());
                n -= rest;
                page_id++;
                while (n >= PAGE_SIZE) {
                    bps = BufpageStream(Bufpage(id, page_id));
                    bps.read_bytes(bytes, PAGE_SIZE);
                    n -= PAGE_SIZE;
                    page_id++;
                }
                if (n) {
                    bps = BufpageStream(Bufpage(id, page_id));
                    bps.read_bytes(bytes, n);
                }
            }
        }
        offset += n;
        return this;
    }

    template<typename T, bool use_buf = true>
    File* read(T& t, size_t n = sizeof(T)) { 
        if constexpr (is_std_vector_v<T>) {
            using size_t = typename T::size_type;
            using value_t = typename T::value_type;
            size_t size;
            File* ret = read<size_t, use_buf>(size);
            t.resize(size);
            for (size_t i = 0; i < size; i++) {
                ret = ret->read<value_t, use_buf>(t[i]);
            }
            return ret;
        } else {
            return read_bytes<use_buf>((bytes_t)&t, n); 
        }
    }

    template<typename T, bool use_buf = true>
    T read() {
        T t;
        read<T, use_buf>(t);
        return t;
    }

    File* seek_pos(size_t pos) { offset = pos; return this; }
    File* seek_off(size_t off) { offset += off; return this; }
};