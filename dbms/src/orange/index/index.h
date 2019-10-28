#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    bool on;
    // 这个属性所占字节数，不包括 rid
    size_t size;
    cnt_t tot;
    String prefix;

    File *f_data;
    // 之后这里可能放一个数据结构而非这个索引文件
    // File *f_idx;

    inline String idx_name() { return prefix + ".idx"; }
    inline String data_name() { return prefix + ".data"; }
    void ensure_size(const byte_arr_t& val) { ensure(val.size() == size, "size check failed in index"); }
public:
    Index(size_t size, String prefix) : size(size), prefix(prefix) {
        on = 0;
        f_data = File::open(data_name());
        // f_idx = nullptr;
    }

    ~Index() {
        f_data->close();
    }

    void turn_on() {
        if (!on) {
            // f_idx = File::open(idx_name());
            on = 1;
            UNIMPLEMENTED
        }
    }

    void turn_off() {
        if (on) {
            // f_idx->close();
            on = 0;
            UNIMPLEMENTED
        }
    }

    // 插入前保证已经得到调整
    void insert(const byte_arr_t& val, rid_t rid) {
        ensure_size(val);
        f_data->seek_pos(rid * sizeof(rid_t))->write_bytes(val.data(), size);
        if (on) {
            UNIMPLEMENTED
        }
        tot++;
    }

    // 调用合适应该不会有问题8
    void remove(rid_t rid) {
        // ensure(f_data->seek_pos(rid * sizeof(rid_t))->read<byte_t>() != DATA_INVALID, "remove record that not exists");
        f_data->seek_pos(rid * sizeof(rid_t))->write<byte_t>(DATA_INVALID);
        if (on) {
            UNIMPLEMENTED
        }
        tot--;
    }

    void update(const byte_arr_t& val, rid_t rid) {
        ensure_size(val);
        f_data->seek_pos(rid * sizeof(rid_t))->write_bytes(val.data(), size);
        if (on) {
            UNIMPLEMENTED
        }
    }

    std::vector<rid_t> get_rid(WhereClause::cmp_t cmp, const byte_arr_t& val) {
        ensure_size(val);
        if (on) {
            UNIMPLEMENTED
        } else {
            std::vector<rid_t> ret;
            f_data->seek_pos(0);
            byte_t bytes[size];
            auto test = [&val, this, cmp] (const_bytes_t bytes){
                int code = strncmp((char*)bytes, (char*)val.data(), size);
                switch (cmp) {
                    case WhereClause::EQ: return code == 0;
                    case WhereClause::LT: return code < 0;
                    case WhereClause::GT: return code > 0;
                    case WhereClause::LE: return code <= 0;
                    case WhereClause::GE: return code >= 0;
                }
            };
            for (cnt_t i = 0; i < tot; i++) {
                f_data->read_bytes(bytes, size);
                if (test(bytes)) ret.push_back(i);
            }
            return ret;
        }
    }

    byte_arr_t get_val(rid_t rid) {
        byte_t bytes[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        return byte_arr_t(bytes, bytes + size);
    }
};
