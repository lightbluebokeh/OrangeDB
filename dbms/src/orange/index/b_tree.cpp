#include <orange/index/b_tree.h>
#include <orange/index/index.h>

using op_t = Orange::parser::op;

void BTree::node_t::check_order() {
#ifdef DEBUG
    for (int i = 0; i + 1 < key_num(); i++) {
        auto& index = tree.index;
        orange_assert(tree.cmp(index.restore(key(i)), val(i), index.restore(key(i + 1)), val(i + 1)) < 0, "btree gua le");
    }
#endif
}

void BTree::insert(const_bytes_t k_raw, rid_t v) {
    if (root->full()) {
        auto s = new_node();
        std::swap(s, root);
        root->ch(0) = s->id;
        split(root, s, 0);
    }
    insert_nonfull(root, k_raw, index.restore(k_raw), v);
}

void BTree::insert_nonfull(node_ptr_t& x, const_bytes_t k_raw, const std::vector<byte_arr_t>& ks, rid_t v) {
    x->check_order();
    int i = upper_bound(x, ks, v);
    orange_assert(!i || cmp(ks, v, index.restore(x->key(i - 1)), x->val(i - 1)) != 0,
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
            if (cmp(ks, v, index.restore(x->key(i)), x->val(i)) > 0) {
                i++;
                y = read_node(x->ch(i));
            }
        }
        insert_nonfull(y, k_raw, ks, v);
    }
    x->check_order();
}

void BTree::remove(const_bytes_t k_raw, rid_t v) {
    remove_nonleast(root, index.restore(k_raw), v);
}

void BTree::remove_nonleast(node_ptr_t& x, const std::vector<byte_arr_t>& ks, rid_t v) {
    x->check_order();
    int i = upper_bound(x, ks, v);
    if (i && cmp(ks, v, index.restore(x->key(i - 1)), x->val(i - 1)) == 0) {
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
                const auto &[k_raw, v] = max_raw(y);
                x->set_key(i, k_raw.data());
                x->val(i) = v;
                x->check_order();
                remove_nonleast(y, index.restore(k_raw.data()), v);
            } else {
                auto z = read_node(x->ch(i + 1));
                if (!z->least()) {
                    auto [k_raw, v] = min_raw(z);
                    x->set_key(i, k_raw.data());
                    x->val(i) = v;
                    x->check_order();
                    remove_nonleast(z, index.restore(k_raw.data()), v);
                } else {
                    merge(x, y, i, std::move(z));
                    remove_nonleast(y, ks, v);
                    if (x->key_num() == 0) std::swap(x, y);
                }
            }
        }
    } else {
        orange_check(!x->leaf(), "trying to delete something does not exist");
        auto y = read_node(x->ch(i));
        if (!y->least())
            remove_nonleast(y, ks, v);
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
                remove_nonleast(y, ks, v);
            else if (i != 0) {
                merge(x, l, i - 1, std::move(y));
                remove_nonleast(l, ks, v);
                if (x->key_num() == 0) std::swap(x, l);
            } else {
                merge(x, y, i, std::move(r));
                remove_nonleast(y, ks, v);
                if (x->key_num() == 0) std::swap(x, y);
            }
        }
    }
    x->check_order();
    d--;
}

int BTree::cmp_key(const std::vector<byte_arr_t>& ks1, const std::vector<byte_arr_t>& ks2) const {
    orange_assert(ks1.size() == ks2.size(), "???");
    for (unsigned i = 0; i < ks1.size(); i++) {
        auto &k1 = ks1[i], &k2 = ks2[i];
        int code = Orange::cmp(k1, k2, index.cols[i].get_datatype_kind());
        if (code != 0) return code;
    }
    return 0;
}

int BTree::upper_bound(const node_ptr_t& x, const std::vector<byte_arr_t>& ks, rid_t v) const {
    // int i = 0;
    // while (i < x->key_num() && index.cmp(k, v, index.restore(x->key(i)), x->val(i)) >= 0) i++;
    int i = x->key_num();
    int l = 0, r = x->key_num() - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        if (cmp(ks, v, index.restore(x->key(m)), x->val(m)) < 0)
            i = m, r = m - 1;
        else
            l = m + 1;
    }
    return i;
}

std::vector<rid_t> BTree::query(const std::vector<preds_t>& preds_list, rid_t lim) const {
    // 确保第一列在 where 中
    orange_assert(!preds_list.front().empty(), "using index without constrainting the first line is stupid");
    std::vector<rid_t> ret;
    query(root, preds_list, ret, lim);
    return ret;
}

bool BTree::query_exists(const node_ptr_t& x, const std::vector<byte_arr_t>& ks) const {
    int i = x->key_num();
    int l = 0, r = x->key_num() - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        int code = cmp_key(ks, index.restore(x->key(m)));
        if (code == 0) return 1;
        if (code < 0)
            i = m, r = m - 1;
        else
            l = m + 1;
    }
    // 此时 i 为 key 的 upper bound
    return !x->leaf() && i ? query_exists(read_node(x->ch(i - 1)), ks) : 0;
}

void BTree::query_eq(const node_ptr_t& x, const std::vector<byte_arr_t>& ks, std::vector<rid_t>& result, rid_t lim) const {
    int i = 0;
    while (i < x->key_num() && cmp_key(ks, index.restore(x->key(i))) < 0) i++;
    node_ptr_t y;
    if (!x->leaf()) {
        y = read_node(x->ch(i));
        query_eq(y, ks, result, lim);
    }
    while (result.size() < lim && i < x->key_num() && cmp_key(ks, index.restore(x->key(i))) == 0) {
        result.push_back(x->val(i));
        if (!x->leaf() && result.size() < lim) {
            y = read_node(x->ch(i + 1));
            query_eq(y, ks, result, lim);
        }
        i++;
    }
}

void BTree::query(const node_ptr_t& x, const std::vector<preds_t>& preds_list, std::vector<rid_t>& result, rid_t lim) const {
    auto check_lo = [&] (int i) {
        for (auto &[op, value]: preds_list.front()) {
            if (ast::is_low(op) && !Orange::cmp(index.restore(x->key(i), 0), index.cols.front().get_datatype_kind(), op, value)) {
                return 0;
            }
        }
        return 1;
    };
    auto check_hi = [&] (int i) {
        for (auto &[op, value]: preds_list.front()) {
            if (ast::is_high(op) && !Orange::cmp(index.restore(x->key(i), 0), index.cols.front().get_datatype_kind(), op, value)) {
                return 0;
            }
        }
        return 1;
    };
    auto check_others = [&] (int i) {
        auto k_raw = x->key(i);
        for (unsigned j = 1; j < preds_list.size(); j++) {
            auto &preds = preds_list[j];
            for (auto &[op, value]: preds) {
                if (!Orange::cmp(index.restore(k_raw, j), index.cols[j].get_datatype_kind(), op, value)) return 0;
            }
        }
        return 1;
    };
    int i = 0;
    while (i < x->key_num() && !check_lo(i)) i++;
    node_ptr_t y;
    if (!x->leaf()) {
        y = read_node(x->ch(i));
        query(y, preds_list, result, lim);
    }
    while (result.size() < lim && i < x->key_num() && check_hi(i)) {
        if (check_others(i)) result.push_back(i);
        if (!x->leaf() && result.size() < lim) {
            y = read_node(x->ch(i + 1));
            query(y, preds_list, result, lim);
        }
        i++;
    }
}
