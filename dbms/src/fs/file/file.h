#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include <defs.h>
#include <fs/file/file_manager.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_manager.h>
#include <fs/bufpage/bufpage_stream.h>

// 打开的文件
class File {
private:
    int id, fd;
    String name;

    File(int id, int fd, const String& name) : id(id), fd(fd), name(name) {}
    File(const File&) = delete;

    ~File() {}

    // void init_metadata(const std::vector<raw_field_t>& raw_fields) {
    //     ensure(raw_fields.size() <= MAX_COL_NUM, "to much fields");
    //     fields.clear();
    //     BufpageStream bps(get_buf_page(0));

    //     record_size = 0;
    //     bps.write<byte_t>(raw_fields.size());
    //     for (auto raw_field : raw_fields) {
    //         auto field = FieldDef::parse(raw_field);
    //         bps.write(field);
    //         record_size += field.get_size();
    //         fields.push_back(std::move(field));
    //     }
    //     bps.memset(0, sizeof(FieldDef) * (MAX_COL_NUM - fields.size()))
    //         .write<uint16_t>(record_size)
    //         .write(record_cnt = 0)
    //         .memset();
    // }

    // void load_metadata() {
    //     fields.clear();
    //     BufpageStream bps(get_buf_page(0));

    //     fields.reserve(bps.read<int>());
    //     for (size_t i = 0; i < fields.capacity(); i++) {
    //         fields.push_back(bps.read<FieldDef>());
    //     }
    //     bps.seekoff(sizeof(FieldDef) * (MAX_COL_NUM - fields.size()))
    //         .read<uint16_t>(record_size)
    //         .read<int>(record_cnt);
    // }

    Bufpage get_buf_page(int page_id) { return Bufpage(id, page_id); }

    static File* file[MAX_FILE_NUM];
    static void close_internal(int id) {
        delete file[id];
        file[id] = nullptr;
    }
public:
    static bool create(const std::string& name) {
        auto fm = FileManager::get_instance();
        ensure(fm->create_file(name.c_str()) == 0, "file create fail");
        return 1;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        auto fm = FileManager::get_instance();
        int id, fd;
        ensure(fm->open_file(name.c_str(), id, fd) == 0, "file open failed");
        if (file[id] == nullptr) file[id] = new File(id, fd, name);
        return file[id];
    }

    // 调用后 f 变成野指针
    static bool close(File* f) {
        auto fm = FileManager::get_instance();
        ensure(fm->close_file(f->id) == 0, "file close failed");
        ensure(file[f->id] == f, "this is magic");
        close_internal(f->id);
        return 1;
    }

    static bool remove(const String& name) {
        auto fm = FileManager::get_instance();
        ensure(fm->remove_file(name) == 0, "remove file failed");
        return 1;
    }

    template<bool use_buf>
    size_t write_bytes(size_t offset, const std::remove_pointer_t<bytes_t>* bytes, size_t n) {
        if constexpr (use_buf == 0) {
            lseek(fd, offset, SEEK_SET);
            ::write(fd, bytes, n);
        } else {
            int page_id = offset >> PAGE_SIZE_IDX;
            offset &= PAGE_SIZE - 1;
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
        return n;
    }

    template<bool use_buf, typename T>
    size_t write(size_t offset, const T& t, size_t n = sizeof(T)) {
        return write_bytes<use_buf>(offset, (bytes_t)&t, n);
    }

    template<bool use_buf>
    size_t read_bytes(size_t offset, bytes_t bytes, size_t n) {
        if constexpr (use_buf == 0) {
            lseek(fd, offset, SEEK_SET);
            ::read(fd, bytes, n);
        } else {
            int page_id = offset >> PAGE_SIZE_IDX;
            offset &= PAGE_SIZE - 1;
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
        return n;
    }

    template< bool use_buf, typename T, typename U = T>
    void read(size_t offset, U& u) { read_bytes<use_buf>(offset, &u, sizeof(T)); }

    template< bool use_buf, typename T>
    T read(size_t offset) {
        static byte_t bytes[sizeof(T)];
        read_bytes<use_buf>(offset, bytes, sizeof(T));
        return *(T*)bytes;
    }
};