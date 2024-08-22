#pragma once
#include "atom.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "utils.h"

enum {
    TOKEN_ATOM=256,
    TOKEN_ARROW,
    TOKEN_RETURN,
    TOKEN_EXTERN,
    // Tokens to stop 
    TOKEN_EOF,
    TOKEN_END=TOKEN_EOF,
    TOKEN_UNPARSABLE,
    TOKEN_COUNT
};
typedef struct {
    const char* path;
    size_t l0, c0;
    size_t l1, c1;
    int kind;
    union {
        Atom* atom;
        uint32_t codepoint;
    };
} Token;
// NOTE: Temporarily display a Token
const char* tdisplay(Token t);
const char* tloc(Token t);
