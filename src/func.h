#pragma once
#include "atom.h"
typedef struct Type Type;
typedef struct {
    Atom* name; // NULL if no name is specified
    Type* type;
} Arg;
typedef struct {
    Arg *items;
    size_t len, cap;
} Args;
typedef struct {
    Args input;
    Type* output;
} FuncSignature;
typedef struct SymTabNode SymTabNode;
typedef struct Statements Statements;
typedef struct {
    // TODO: type is kind of unnecessary
    Type* type;
    Statements* scope;
    SymTabNode* symtab_node;
} Function;
Function* func_new(Arena* arena, Type* type, Statements* scope);
