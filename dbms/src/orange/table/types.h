#pragma once

#include <defs.h>
#include <cstring>

struct Type {
    enum Kind {
        INT,
        CHAR,
        VARCHAR,
        NUMERIC,
        DATE,
    };
    Kind kind;
    int size;
    int p = 18, s = 0;  // kind == NUMERIC 才有效

    // 保证以 0 结尾 233
    static Type parse(const String& raw_type) {
        int size, p, s;
        if (sscanf(raw_type.data(), "INT(%d)", &size) == 1) {
            return {INT, 4};
        } else if (sscanf(raw_type.data(), "VARCHAR(%d)", &size) == 1) {
            return {VARCHAR, size};
        } else if (sscanf(raw_type.data(), "CHAR(%d)", &size) == 0) {
            return {CHAR, size};
        } else if (strcmp(raw_type.data(), "DATE") == 0) {
            return {DATE, 2};
        } else if (strcmp(raw_type.data(), "NUMERIC") == 0) {
            return {NUMERIC, 8};
        } else if (sscanf(raw_type.data(), "NUMERIC(%d)", &p) == 1) {
            return {NUMERIC, 8, p};
        } else if (sscanf(raw_type.data(), "NUMERIC(%d,%d)", &p, &s) == 2) {
            return {NUMERIC, 8, p, s};
        } else {
            throw "ni zhe shi shen me dong xi";
        }
    }
};
