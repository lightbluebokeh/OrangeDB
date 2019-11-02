#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/index/b_tree.h>

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    index_key_kind_t kind;
    size_t size;
    String prefix;

    //  meta
    bool on;

    File* f_data;
    BTree *tree;

    String data_name() { return prefix + ".data"; }
    String meta_name() { return prefix + ".meta"; }

    void read_meta() {
        std::ifstream is(meta_name());
        is >> on;
    }
    void write_meta() {
        std::ofstream os(meta_name());
        os << on;
    }
public:
    Index(index_key_kind_t kind, size_t size, const String& prefix) : kind(kind), size(size), prefix(prefix) {}
    ~Index() {
        write_meta();
        f_data->close();
        if (on) turn_off();
    }

    void init() {
        f_data = File::create_open(data_name());
        on = 0;
    }
    void load() {
        f_data = File::open(data_name());
        read_meta();
        if (on) {
            tree = new BTree(kind, size, prefix);
            tree->load();
        }
    }

    void turn_on() {
        if (!on) {
            on = 1;
            tree = new BTree(kind, size, prefix);
            tree->init(f_data);
        }
    }

    void turn_off() {
        if (on) {
            on = 0;
            delete tree;
        }
    }

    // 插入前保证已经得到调整
    void insert(const_bytes_t val, rid_t rid) {
        if (on) tree->insert(val, rid);
        f_data->seek_pos(rid * sizeof(rid_t))->write_bytes(val, size);
    }

    // 调用合适应该不会有问题8
    void remove(rid_t rid) {
        if (on) {
            bytes_t bytes = new byte_t[size];
            f_data->seek_pos(rid * size)->read_bytes(bytes, size);
            tree->remove(bytes, rid);
            delete[] bytes;
        }
        if (f_data->size() == rid * size) f_data->resize((rid - 1) * size);
        else f_data->seek_pos(rid * size)->write<byte_t>(DATA_INVALID);
    }

    void update(const_bytes_t val, rid_t rid) {
        if (on) {
            bytes_t bytes = new byte_t[size];
            f_data->read_bytes(bytes, size);
            tree->remove(bytes, rid);
            tree->insert(val, rid);
            delete[] bytes;
        }
        f_data->seek_pos(rid * size)->write_bytes(val, size);
    }

    std::vector<rid_t> get_rid(const_bytes_t lo, const_bytes_t hi) {
        
    }

    byte_arr_t get_val(rid_t rid) {
        byte_arr_t bytes(size);
        f_data->seek_pos(rid * size)->read_bytes(bytes.data(), size);
        return bytes;
    }
};
