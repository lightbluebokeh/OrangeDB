#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/index/b_tree.h>

class Table;

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    Table& table;

    data_kind_t kind;
    size_t size;
    String prefix;

    bool on;

    File* f_data;
    BTree* tree;

    String data_name() { return prefix + ".data"; }
    String meta_name() { return prefix + ".meta"; }

    byte_arr_t store(const byte_arr_t& key) {
        switch (kind) {
            case ORANGE_VARCHAR: UNIMPLEMENTED
            default: return key;
        }
    }
    byte_arr_t restore(const_bytes_t k_raw) {
        switch (kind) {
            case ORANGE_DATE:
            case ORANGE_INT:
            case ORANGE_CHAR: return byte_arr_t(k_raw, k_raw + size);
            case ORANGE_VARCHAR: UNIMPLEMENTED
            default: UNIMPLEMENTED
        }
    }

    int cmp_key(const byte_arr_t& k1, const byte_arr_t& k2) const {
        switch (kind) {
            case ORANGE_DATE:
            case ORANGE_CHAR: return bytesncmp(k1.data(), k2.data(), size);
            case ORANGE_INT:
                if (k1.front() != k2.front()) return int(k1.front()) - int(k2.front());
                return *(int*)(k1.data() + 1) - *(int*)(k2.data() + 1);
            default: UNIMPLEMENTED
        }
    }

    int cmp(const byte_arr_t& k1, rid_t v1, const byte_arr_t& k2, rid_t v2) const {
        int key_code = cmp_key(k1, k2);
        return key_code == 0 ? v1 - v2 : key_code;
    }

    bool test_pred_lo(const byte_arr_t& k, const pred_t& pred) {
        int code = cmp_key(pred.lo, k);
        if (code == 0) {
            int a = 1;
            a++;
        }
        if (code < 0 || (pred.lo_eq && code == 0)) {
            return 1;
        } else {
            return 0;
        }
    }

    bool test_pred_hi(const byte_arr_t& k, const pred_t& pred) {
        int code = cmp_key(k, pred.hi);
        if (code == 0) {
            int a = 1;
            a++;
        }
        if (code < 0 || (pred.hi_eq && code == 0)) {
            return 1;
        } else {
            return 0;
        }
    }

    bool test_pred(const byte_arr_t& k, const pred_t& pred) {
        if (test_pred_lo(k, pred) && test_pred_hi(k, pred)) {
            return 1;
        } else {
            return 0;
        }
    }

    // 返回 table 中的最大编号，f**k c++
    rid_t get_tot();
    byte_arr_t get_raw(rid_t rid) {
        auto bytes = new byte_t[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        auto ret = byte_arr_t(bytes, bytes + size);
        delete[] bytes;
        return ret;
    }

public:
    Index(Table& table, data_kind_t kind, size_t size, const String& prefix, bool on) :
        table(table), kind(kind), size(size), prefix(prefix), on(on) {
        if (!fs::exists(data_name())) File::create(data_name());
        f_data = File::open(data_name());
    }
    Index(Index&& index) :
        table(index.table), kind(index.kind), size(index.size), prefix(index.prefix), on(index.on),
        f_data(index.f_data), tree(index.tree) {
        if (on) tree->index = this;
        index.f_data = nullptr;
        index.on = 0;
    }
    ~Index() {
        if (on) delete tree;
        if (f_data) f_data->close();
    }

    void load() {
        if (on) {
            tree = new BTree(this, size, prefix);
            tree->load();
        }
    }

    void turn_on() {
        if (!on) {
            on = 1;
            tree = new BTree(this, size, prefix);
            tree->init(f_data);
        }
    }

    void turn_off() {
        if (on) {
            on = 0;
            delete tree;
        }
    }

    void insert(const byte_arr_t& val, rid_t rid) {
        auto raw = store(val);
        if (on) tree->insert(raw.data(), rid, val);
        f_data->seek_pos(rid * size)->write_bytes(raw.data(), size);
    }

    // 调用合适应该不会有问题8
    void remove(rid_t rid) {
        if (on) {
            auto raw = get_raw(rid);
            tree->remove(raw.data(), rid);
        }
        f_data->seek_pos(rid * size)->write<byte_t>(DATA_INVALID);
    }

    void update(const byte_arr_t& val, rid_t rid) {
        remove(rid);
        insert(val, rid);
    }

    // lo_eq 为真表示允许等于
    std::vector<rid_t> get_rid(const pred_t& pred, rid_t lim) {
        if (on)
            return tree->query(pred, lim);
        else {
            std::vector<rid_t> ret;
            bytes_t bytes = new byte_t[size];
            f_data->seek_pos(0);
            for (rid_t i = 0, tot = get_tot(); i < tot && lim; i++) {
                f_data->read_bytes(bytes, size);
                if (*bytes != DATA_INVALID && test_pred(restore(bytes), pred)) {
                    ret.push_back(i);
                    lim--;
                }
            }
            delete[] bytes;
            return ret;
        }
    }

    byte_arr_t get_val(rid_t rid) {
        auto bytes = new byte_t[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        auto ret = restore(bytes);
        delete[] bytes;
        return ret;
    }

    friend class BTree;
};
