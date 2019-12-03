#pragma once

#include "defs.h"
#include "orange/parser/sql_ast.h"

static String to_string(datatype_t type) {
    switch (type) {
        case ORANGE_INT: return "int";
        case ORANGE_NUMERIC: return "numeric";
        case ORANGE_CHAR:
        case ORANGE_VARCHAR: return "string";
        case ORANGE_DATE: return "date";
    }
    return "fuck warning";
}

namespace Orange {
    // 直接按照 op 比较
    template<typename T1, typename T2>
    inline bool cmp(const T1& t1, Orange::parser::op op, const T2& t2) {
        using op_t = decltype(op);
        switch (op) {
            case op_t::Eq: return t1 == t2;
            case op_t::Ge: return t1 >= t2;
            case op_t::Gt: return t1 > t2;
            case op_t::Le: return t1 <= t2;
            case op_t::Lt: return t1 < t2;
            case op_t::Neq: return t1 != t2;
        }
        return (bool)"fuck warning";
    }

    // bytes 与 value 按照 op 比较
    inline bool cmp(const byte_arr_t& v1_bytes, datatype_t t1, Orange::parser::op op, const Orange::parser::data_value& v2) {
        using op_t = Orange::parser::op;
        orange_assert(!v2.is_null(), "value should not be null here");
        if (v1_bytes.front() == DATA_NULL) {
            return op == op_t::Le || op == op_t::Lt || op == op_t::Neq;
        }
        switch (t1) {
            case ORANGE_INT: {
                int v1 = bytes_to_int(v1_bytes);
                if (v2.is_int()) return cmp(v1, op, v2.to_int());
                else if (v2.is_float()) {
                    ORANGE_UNIMPL
                } else {
                    ORANGE_UNREACHABLE
                }
            } break;
            case ORANGE_VARCHAR:
            case ORANGE_CHAR: {
                auto v1 = bytes_to_string(v1_bytes);
                if (v2.is_string()) return cmp(v1, op, v2.to_string());
                else {
                    ORANGE_UNREACHABLE
                }
            } break;
            case ORANGE_NUMERIC: {
                ORANGE_UNIMPL
            } break;
            case ORANGE_DATE: {
                ORANGE_UNIMPL
            } break;
        }
        return (bool)"fuck warning";
    }

    // bytes 和 bytes 按照 op 比较
    inline bool cmp(const byte_arr_t& v1_bytes, datatype_t t1, Orange::parser::op op, const byte_arr_t& v2_bytes, datatype_t t2) {
        using op_t = Orange::parser::op;
        if (v1_bytes.front() == DATA_NULL) {
            switch (op) {
                case op_t::Le:return true;
                case op_t::Eq:
                case op_t::Ge: return v2_bytes.front() == DATA_NULL;
                case op_t::Lt:
                case op_t::Neq: return v2_bytes.front() != DATA_NULL;
                case op_t::Gt: return false;
            }
        }
        // 此时 v1　一定不是 null
        if (v2_bytes.front() == DATA_NULL) return op == op_t::Neq || op == op_t::Gt || op == op_t::Ge;
        switch (t1) {
            case ORANGE_INT: {
                int v1 = Orange::bytes_to_int(v1_bytes);
                switch (t2) {
                    case ORANGE_INT: {
                        return cmp(v1, op, bytes_to_int(v2_bytes));
                    } break;
                    case ORANGE_NUMERIC:
                        ORANGE_UNIMPL
                    break;
                    default: 
                        throw OrangeError("types `" + to_string(t1) + "` and `" + to_string(t2) + "` are not comparable");
                }
            } break;
            case ORANGE_VARCHAR:
            case ORANGE_CHAR: {
                String v1 = Orange::bytes_to_string(v1_bytes);
                switch (t2) {
                    case ORANGE_CHAR:
                    case ORANGE_VARCHAR: {
                        int code = v1.compare(Orange::bytes_to_string(v2_bytes));
                        switch (op) {
                            case op_t::Eq: return code == 0;
                            case op_t::Ge: return code >= 0;
                            case op_t::Gt: return code > 0;
                            case op_t::Le: return code <= 0;
                            case op_t::Lt: return code < 0;
                            case op_t::Neq: return code != 0;
                        }
                    } break;
                    default:
                        throw OrangeError("types `" + to_string(t1) + "` and `" + to_string(t2) + "` are not comparable");
                }
            } break;
            case ORANGE_NUMERIC:
                ORANGE_UNIMPL
            case ORANGE_DATE:
                ORANGE_UNIMPL
        }
        ORANGE_UNREACHABLE
        return 1;
    }

    // 同种 bytes 比较
    inline int cmp(const byte_arr_t& k1, const byte_arr_t& k2, datatype_t kind) {
        if (k1.front() != k2.front()) return int(k1.front()) - int(k2.front());
        switch (kind) {
            case ORANGE_INT: return *(int*)(k1.data() + 1) - *(int*)(k2.data() + 1);
            case ORANGE_CHAR:
                for (int i = 1; i < int(k1.size()); i++) {
                    if (k1[i] != k2[i]) return int(k1[i]) - int(k2[i]);
                }
                return 0;
            case ORANGE_VARCHAR:
                for (int i = 1; i < int(std::min(k1.size(), k2.size())); i++) {
                    if (k1[i] != k2[i]) return int(k1[i]) - int(k2[i]);
                }
                if (k1.size() == k2.size()) return 0;
                return k1.size() < k2.size() ? -int(k2[k1.size()]) : k1[k2.size()];
            default: ORANGE_UNIMPL
        }
    }
};

struct pred_t {
    byte_arr_t lo;
    bool lo_eq;  // 是否允许取等
    byte_arr_t hi;
    bool hi_eq;

    bool test_lo(const byte_arr_t& k, datatype_t kind) const {
        auto code = Orange::cmp(lo, k, kind);
        return code < 0 || (code == 0 && lo_eq);
    }

    bool test_hi(const byte_arr_t& k, datatype_t kind) const {
        auto code = Orange::cmp(k, hi, kind);
        return code < 0 || (code == 0 && hi_eq);
    }

    bool test(const byte_arr_t& k, datatype_t kind) const {
        return test_lo(k, kind) && test_hi(k, kind);
    }
};