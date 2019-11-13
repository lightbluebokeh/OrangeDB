#include <orange/index/index.h>
#include <orange/index/b_tree.h>

#ifdef DEBUG
void BTree::node_t::check_order() {
    for (int i = 0; i + 1 < key_num(); i++) {
        auto &index = *tree.index;
        assert(index.cmp(index.restore(key(i)), val(i), index.restore(key(i + 1)), val(i + 1)) < 0);
    }
}
#endif

void BTree::remove_nonleast(node_ptr_t& x, const byte_arr_t& k, rid_t v) {
#ifdef DEBUG
    x->check_order();
#endif
    int i = upper_bound(x, k, v);
    if (i && index->cmp(k, v, index->restore(x->key(i - 1)), x->val(i - 1)) == 0) {
        i--;
        if (x->leaf()) {
            for (int j = i; j + 1 < x->key_num(); j++) {
                x->set_key(j, x->key(j + 1));
                x->val(j) = x->val(j + 1);
            }
            x->key_num()--;
#ifdef DEBUG
            x->check_order();
#endif
        } else {
            auto y = read_node(x->ch(i));
            if (!y->least()) {
                auto [k_raw, v] = max_raw(y);
                x->set_key(i, k_raw.data());
                x->val(i) = v;
#ifdef DEBUG
                x->check_order();
#endif
                remove_nonleast(y, index->restore(k_raw.data()), v);
            } else {
                auto z = read_node(x->ch(i + 1));
                if (!z->least()) {
                    auto [k_raw, v] = min_raw(z);
                    x->set_key(i, k_raw.data());
                    x->val(i) = v;
#ifdef DEBUG
                    x->check_order();
#endif
                    remove_nonleast(z, index->restore(k_raw.data()), v);
                } else {
                    merge(x, y, i, std::move(z));
                    remove_nonleast(y, k, v);
                    if (x->key_num() == 0) std::swap(x, y);
                }
            }
        } 
    } else {
        ensure(!x->leaf(), "trying to delete something does not exist");
        auto y = read_node(x->ch(i));
        if (!y->least()) remove_nonleast(y, k, v);
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
#ifdef DEBUG
                x->check_order();
                y->check_order();
                l->check_order();
#endif
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
#ifdef DEBUG
                x->check_order();
                y->check_order();
                r->check_order();
#endif                
                return 1;
            };

            if (from_l() || from_r()) remove_nonleast(y, k, v);
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
#ifdef DEBUG
    x->check_order();
#endif
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

int BTree::upper_bound(node_ptr_t &x, const byte_arr_t& k, rid_t v) {
    // int i = 0;
    // while (i < x->key_num() && index->cmp(k, v, index->restore(x->key(i)), x->val(i)) >= 0) i++;
    int i = x->key_num();
    int l = 0, r = x->key_num() - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        if (index->cmp(k, v, index->restore(x->key(m)), x->val(m)) < 0) i = m, r = m - 1;
        else l = m + 1;
    }
    return i;
}

void BTree::insert_nonfull(node_ptr_t &x, const_bytes_t k_raw, const byte_arr_t& k, rid_t v) {
#ifdef DEBUG
    x->check_order();
#endif
    int i = upper_bound(x, k, v);
    ensure(!i || index->cmp(k, v, index->restore(x->key(i - 1)), x->val(i - 1)) != 0, "try to insert something already exists");
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
            if (index->cmp(k, v, index->restore(x->key(i)), x->val(i)) > 0) {
                i++;
                y = read_node(x->ch(i));
            }
        }
        insert_nonfull(y, k_raw, k, v);
    }
#ifdef DEBUG
    x->check_order();
#endif
}

void BTree::query(node_ptr_t& x, const pred_t& pred, std::vector<rid_t>& ret, rid_t& lim) {
    int i = 0;
    while (i < x->key_num() && !index->test_pred_lo(index->restore(x->key(i)), pred)) 
        i++;
    node_ptr_t y;
    if (!x->leaf()) {
        y = read_node(x->ch(i));
        query(y, pred, ret, lim);
    }
    while (lim && i < x->key_num() && index->test_pred_hi(index->restore(x->key(i)), pred)) {
        ret.push_back(x->val(i));
        lim--;
        if (lim && !x->leaf()) {
            y = read_node(x->ch(i + 1));
            query(y, pred, ret, lim);
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
    for (rid_t i = 0, tot = index->get_tot(); i < tot; i++) {
        f_data->read_bytes(k_raw, key_size);
        if (*k_raw != DATA_INVALID) insert(k_raw, i, index->restore(k_raw));
    }
    delete k_raw;
}
