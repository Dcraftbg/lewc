#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "darray.h"
#include "utils.h"
// NOTE: func.h depends on typeid_t to be defined
typedef size_t typeid_t;
#include "func.h"
enum {
    CORE_I32,
    CORE_FUNC,
};
typedef struct {
    const char* name;
    int core;
    union {
       bool unsign; // Is it unsigned?
       FuncSignature signature;
    };
} Type;
typedef struct {
    Type *items;
    size_t len, cap;
} TypeTable;
#define INVALID_TYPEID ((typeid_t)-1L)
typeid_t type_table_get_by_name(TypeTable* t, const char* name);
Type* type_table_get(TypeTable* t, typeid_t id);
typeid_t type_table_create(TypeTable* t, Type type);
void type_table_move(TypeTable* into, TypeTable* from);
void type_table_init(TypeTable* t);
