#pragma once
#include "atom.h"
#include "location.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "utils.h"
enum {
    TOKEN_ATOM=256,
    TOKEN_ARROW,
    TOKEN_RETURN,
    TOKEN_EXTERN,
    TOKEN_TYPEDEF,
    TOKEN_STRUCT,
    TOKEN_C_STR,
    TOKEN_STR,
    TOKEN_INT,
    TOKEN_EQEQ,
    TOKEN_NEQ,
    TOKEN_SHR,
    TOKEN_SHL,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_BOOL_AND,
    TOKEN_BOOL_OR,
    TOKEN_NULL,
    TOKEN_CAST,
    TOKEN_DEFER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_SIZE_OF,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_LOOP,
    // Tokens to stop 
    TOKEN_END,
    TOKEN_EOF=TOKEN_END,
    TOKEN_ERR,
    TOKEN_UNPARSABLE=TOKEN_ERR,
    TOKEN_INVALID_STR,
    TOKEN_INVALID_INT_LITERAL,
    TOKEN_COUNT
};
typedef struct Type Type;
typedef struct {
    Location loc;
    int kind;
    union {
        Atom* atom;
        struct {
            const char* str;
            size_t str_len;
        };
        uint32_t codepoint;
        struct {
            Type* type;
            uint64_t value;
        } integer;
    };
    /*Small string buffer?*/
} Token;
// NOTE: Temporarily display a Token
const char* tdisplay(Token t);
