#pragma once

#include <defs.h>

class FieldDef {
public:
    enum class TypeKind {
        INT,
        VARCHAR,
        FLOAT,
        DATE,
    };

    struct Type {
        TypeKind kind;
        int size;

        // 保证以 0 结尾 233
        static Type parse_bytes(bytes_t bytes) {
            int size;
            if (sscanf((char*)bytes, "INT(%d)", &size) == 1) {
                return {TypeKind::INT, size};
            } else if (sscanf((char*)bytes, "VARCHAR(%d)", &size) == 1) {
                return {TypeKind::VARCHAR, size};
            } else if (strcmp((char*)bytes, "FLOAT") == 0) {
                return {TypeKind::FLOAT, 4};
            } else if (strcmp((char*)bytes, "DATE") == 0) {
                return {TypeKind::DATE, size};
            } else {
                throw "type parse error";
            }
        }

        ByteArr to_bytes() {
            switch (kind) {
                case TypeKind::INT:
                    return (bytes_t)("INT(" + std::to_string(size) + ")").c_str();
                break;

                case TypeKind::VARCHAR:
                    return (bytes_t)("VARCHAR(" + std::to_string(size) + ")").c_str();
                break;

                case TypeKind::FLOAT:
                    return (bytes_t)"FLOAT";
                break;
            
                case TypeKind::DATE:
                    return (bytes_t)"DATE";
                break;
            }
        }
    };
private:
    String name;
    Type type;

    FieldDef(const String& name, const Type& type) : name(name), type(type) {}
public:
    int get_size() { return type.size; }

    static FieldDef parse(const std::pair<String, String>& name_type) {
        return {name_type.first, Type::parse_bytes((bytes_t)name_type.second.c_str())};
    }

    static FieldDef parse_bytes(const ByteArr& arr) {
        ensure(arr.size() == COL_SIZE, "parse field failed, go fix your bug");
        int pos = arr.find(' ');
        return {String((char*)arr.data(), pos), Type::parse_bytes((bytes_t)arr.c_str() + pos)};
    }

    // 用空格作分隔符了，名字里请不要带空格 /cy
    ByteArr to_bytes() {
        auto ret = ByteArr((bytes_t)name.c_str()) + (bytes_t)" " + type.to_bytes();
        // 至少还得留一个存 0
        ensure(ret.size() <= MAX_COL_NUM, "field def too long");
        return ret + ByteArr(MAX_COL_NUM - ret.size(), 0);
    }
};
