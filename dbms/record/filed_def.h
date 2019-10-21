#pragma once

#include <defs.h>

constexpr int MAX_FIELD_LENGTH = 32;

// raw type
typedef struct {
    String name;
    String raw_type;
} raw_field_t;

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
        static Type parse(const String& raw_type) {
            int size;
            if (sscanf(raw_type.data(), "INT(%d)", &size) == 1) {
                return {TypeKind::INT, size};
            }
            else if (sscanf(raw_type.data(), "VARCHAR(%d)", &size) == 1) {
                return {TypeKind::VARCHAR, size};
            }
            else if (strcmp(raw_type.data(), "FLOAT") == 0) {
                return {TypeKind::FLOAT, 4};
            }
            else if (strcmp(raw_type.data(), "DATE") == 0) {
                return {TypeKind::DATE, size};
            }
            else {
                throw "fail to parse type: " + raw_type;
            }
        }
    };

private:
    char name[MAX_FIELD_LENGTH + 1];
    Type type;

    // static constexpr int a = sizeof(Type)

    FieldDef(const String& name, const Type& type) : type(type) {
        ensure(name.length() <= MAX_FIELD_LENGTH, "field name to long: " + name);

        memcpy(this->name, name.data(), name.length() + 1);
    }

public:
    int get_size() { return type.size; }

    static FieldDef parse(raw_field_t raw_field) {
        return {raw_field.name, Type::parse(raw_field.raw_type)};
    }
};
