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
