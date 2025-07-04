#pragma once
#include "atom.h"
#include "location.h"
typedef struct Type Type;
// TODO: Symbol instead of this maybe
typedef struct {
    Atom* name; // NULL if no name is specified
    Type* type;
    Location loc;
} Arg;
typedef struct {
    Arg *items;
    size_t len, cap;
} Args;
typedef struct {
    Args input;
    enum {
        VARIADIC_NONE,
        VARIADIC_C
    } variadic;
    Type* output;
} FuncSignature;
typedef struct SymTabNode SymTabNode;
typedef struct Statements Statements;
// TODO: Maybe even remove this entirely and just have this inside the ast
typedef struct {
    // TODO: type is kind of unnecessary
    Type* type;
    Statements* scope;
    SymTabNode* symtab_node;
} Function;
Function* func_new(Arena* arena, Type* type, Statements* scope);
