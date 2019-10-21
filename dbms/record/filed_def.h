#pragma once

#include <defs.h>
#include <cstring>
#include <record/types.h>

constexpr int MAX_FIELD_LENGTH = 32;

// raw type
typedef struct {
    String name;
    String raw_type;
} raw_field_t;

class FieldDef {
private:
    char name[MAX_FIELD_LENGTH + 1];
    Type type;

    FieldDef(const String& name, const String& raw_type) : type(Type::parse(raw_type)) {
        memcpy(this->name, name.data(), name.length() + 1);
    }

public:
    int get_size() { return type.size; }

    static FieldDef parse(raw_field_t raw_field) {
        return {raw_field.name, raw_field.raw_type};
    }
};
