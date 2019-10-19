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

        static Type parse(String x);

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

        Type parse_bytes(const bytes_t bytes) {

        }
    };
private:
    String name;
    Type type;

    FieldDef(const String& name, const Type& type) : name(name), type(type) {}
public:
    int get_size() { return type.size; }

    static FieldDef parse(const std::pair<String, String>& name_type) {
        return {name_type.first, Type::parse(name_type.second)};
    }

    static FieldDef parse_bytes(const ByteArr& arr) {
        ensure(arr.size() == COL_SIZE, "parse field failed, go fix your bug");
        int pos = arr.find(' ');
    }

    // 用空格作分隔符了，名字里请不要带空格 /cy
    ByteArr to_bytes() {
        auto ret = ByteArr((bytes_t)name.c_str()) + (bytes_t)" " + type.to_bytes();
        // 还得留一个存 0
        ensure(ret.size() < MAX_COL_NUM, "field def too long");
        ret += ByteArr(MAX_COL_NUM - ret.size(), 0);
    }
};
