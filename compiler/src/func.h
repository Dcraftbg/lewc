#pragma once
#include "type.h"
#include "atom.h"
typedef struct {
    Atom* name; // NULL if no name is specified
    typeid_t typeid;
} Arg;
typedef struct {
    Arg *items;
    size_t len, cap;
} Args;
typedef struct {
    Args input;
    typeid_t output;
} FuncSignature;
