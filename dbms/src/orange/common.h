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
    }

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
    }

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
};