#include <orange/index/b_tree.h>
#include <orange/index/index.h>

using op_t = Orange::parser::op;

void BTree::node_t::check_order() {
    for (int i = 0; i + 1 < key_num(); i++) {
        auto& index = *tree.index;
        orange_assert(tree.cmp(index.restore(key(i)), val(i), index.restore(key(i + 1)), val(i + 1)) < 0, "btree gua le");
    }
}

void BTree::remove_nonleast(node_ptr_t& x, const byte_arr_t& k, rid_t v) {
    x->check_order();
    int i = upper_bound(x, k, v);
    if (i && cmp(k, v, index->restore(x->key(i - 1)), x->val(i - 1)) == 0) {
        i--;
        if (x->leaf()) {
            for (int j = i; j + 1 < x->key_num(); j++) {
                x->set_key(j, x->key(j + 1));
                x->val(j) = x->val(j + 1);
            }
            x->key_num()--;
            x->check_order();
        } else {
            auto y = read_node(x->ch(i));
            if (!y->least()) {
                auto [k_raw, v] = max_raw(y);
                x->set_key(i, k_raw.data());
                x->val(i) = v;
                x->check_order();
                remove_nonleast(y, index->restore(k_raw.data()), v);
            } else {
                auto z = read_node(x->ch(i + 1));
                if (!z->least()) {
                    auto [k_raw, v] = min_raw(z);
                    x->set_key(i, k_raw.data());
                    x->val(i) = v;
                    x->check_order();
                    remove_nonleast(z, index->restore(k_raw.data()), v);
                } else {
                    merge(x, y, i, std::move(z));
                    remove_nonleast(y, k, v);
                    if (x->key_num() == 0) std::swap(x, y);
                }
            }
        }
    } else {
        orange_ensure(!x->leaf(), "trying to delete something does not exist");
        auto y = read_node(x->ch(i));
        if (!y->least())
            remove_nonleast(y, k, v);
        else {
            node_ptr_t l, r;
            auto from_l = [&] {
                if (i == 0) return 0;
                l = read_node(x->ch(i - 1));
                if (l->least()) return 0;
                y->ch(t) = y->ch(t - 1);
                for (int j = t - 1; j > 0; j--) {
                    y->set_key(j, y->key(j - 1));
                    y->val(j) = y->val(j - 1);
                    y->ch(j) = y->ch(j - 1);
                }
                y->set_key(0, x->key(i - 1));
                y->val(0) = x->val(i - 1);
                y->ch(0) = l->ch(l->key_num());
                y->key_num()++;
                x->set_key(i - 1, l->key(l->key_num() - 1));
                x->val(i - 1) = l->val(l->key_num() - 1);
                l->key_num()--;
                x->check_order();
                y->check_order();
                l->check_order();
                return 1;
            };
            auto from_r = [&] {
                if (i == x->key_num()) return 0;
                r = read_node(x->ch(i + 1));
                if (r->least()) return 0;
                y->set_key(y->key_num(), x->key(i));
                y->val(y->key_num()) = x->val(i);
                y->ch(y->key_num() + 1) = r->ch(0);
                y->key_num()++;
                x->set_key(i, r->key(0));
                x->val(i) = r->val(0);
                for (int j = 0; j + 1 < r->key_num(); j++) {
                    r->ch(j) = r->ch(j + 1);
                    r->set_key(j, r->key(j + 1));
                    r->val(j) = r->val(j + 1);
                }
                r->ch(r->key_num() - 1) = r->ch(r->key_num());
                r->key_num()--;
                x->check_order();
                y->check_order();
                r->check_order();
                return 1;
            };

            if (from_l() || from_r())
                remove_nonleast(y, k, v);
            else if (i != 0) {
                merge(x, l, i - 1, std::move(y));
                remove_nonleast(l, k, v);
                if (x->key_num() == 0) std::swap(x, l);
            } else {
                merge(x, y, i, std::move(r));
                remove_nonleast(y, k, v);
                if (x->key_num() == 0) std::swap(x, y);
            }
        }
    }
    x->check_order();
}

void BTree::insert(const_bytes_t k_raw, rid_t v, const byte_arr_t& k) {
    if (root->full()) {
        auto s = new_node();
        std::swap(s, root);
        root->ch(0) = s->id;
        split(root, s, 0);
    }
    insert_nonfull(root, k_raw, k, v);
}

void BTree::remove(const_bytes_t k_raw, rid_t v) {
    remove_nonleast(root, index->restore(k_raw), v);
}

int BTree::cmp(const byte_arr_t& k1, rid_t v1, const byte_arr_t& k2, rid_t v2) const {
    int key_code = Orange::cmp(k1, k2, index->kind);
    return key_code == 0 ? v1 - v2 : key_code;
}

int BTree::upper_bound(node_ptr_t& x, const byte_arr_t& k, rid_t v) {
    // int i = 0;
    // while (i < x->key_num() && index->cmp(k, v, index->restore(x->key(i)), x->val(i)) >= 0) i++;
    int i = x->key_num();
    int l = 0, r = x->key_num() - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        if (cmp(k, v, index->restore(x->key(m)), x->val(m)) < 0)
            i = m, r = m - 1;
        else
            l = m + 1;
    }
    return i;
}

void BTree::insert_nonfull(node_ptr_t& x, const_bytes_t k_raw, const byte_arr_t& k, rid_t v) {
    x->check_order();
    int i = upper_bound(x, k, v);
    orange_ensure(!i || cmp(k, v, index->restore(x->key(i - 1)), x->val(i - 1)) != 0,
           "try to insert something already exists");
    if (x->leaf()) {
        for (int j = x->key_num() - 1; j >= i; j--) {
            x->set_key(j + 1, x->key(j));
            x->val(j + 1) = x->val(j);
        }
        x->set_key(i, k_raw);
        x->val(i) = v;
        x->key_num()++;
    } else {
        auto y = read_node(x->ch(i));
        if (y->full()) {
            split(x, y, i);
            if (cmp(k, v, index->restore(x->key(i)), x->val(i)) > 0) {
                i++;
                y = read_node(x->ch(i));
            }
        }
        insert_nonfull(y, k_raw, k, v);
    }
    x->check_order();
}

void BTree::query_internal(node_ptr_t &x, Orange::parser::op op, const Orange::parser::data_value& value, std::vector<rid_t>& ret, rid_t lim) {
    if (ret.size() >= lim) return;
    orange_assert(op != op_t::Neq, "btree is not able to query neq directly");
    int i = 0;
    // 有下界
    if (op == op_t::Ge || op == op_t::Gt || op == op_t::Eq) {
        while (i < x->key_num() && !Orange::cmp(index->restore(x->key(i)), index->kind, op, value)) i++;
    }
    node_ptr_t y;
    if (!x->leaf()) {
        y = read_node(x->ch(i));
        query_internal(y, op, value, ret, lim);
    }
    while (ret.size() < lim && Orange::cmp(index->restore(x->key(i)), index->kind, op, value)) {
        ret.push_back(x->val(i));
        if (!x->leaf()) {
            y = read_node(x->ch(i + 1));
            query_internal(y, op, value, ret, lim);
        }
        i++;
    }
}

void BTree::init(File* f_data) {
    f_tree = File::create_open(tree_name());
    pool.init();

    root = new_node();
    bytes_t k_raw = new byte_t[key_size];
    f_data->seek_pos(0);
    std::cerr << "B-tree init:" << std::endl;
    const auto &all = index->get_all();
    for (auto i: all) {
        f_data->seek_pos(i * key_size)->read_bytes(k_raw, key_size);
        insert(k_raw, i, index->restore(k_raw));
        std::cerr << '\r' << i << "/" << all.size();
    }
    std::cerr << std::endl;
    delete[] k_raw;
}
