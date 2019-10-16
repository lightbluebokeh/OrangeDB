#pragma once

#include <defs.h>
#include <record/bytes.h>

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

        Bytes to_bytes() {
            switch (kind) {
                case TypeKind::INT:
                    return (buf_t)("INT(" + std::to_string(size) + ")").c_str();
                break;

                case TypeKind::VARCHAR:
                    return (buf_t)("VARCHAR(" + std::to_string(size) + ")").c_str();
                break;

                case TypeKind::FLOAT:
                    return (buf_t)"FLOAT";
                break;
            
                case TypeKind::DATE:
                    return (buf_t)"DATE";
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
        return {name_type.first, Type::parse(name_type.second)};
    }

    Bytes to_bytes() {
        Bytes ret;
        ret.clear();
        for (byte_t c: name) {
            ret.push_back(c);
        }
        return ret + type.to_bytes();
    }
};
