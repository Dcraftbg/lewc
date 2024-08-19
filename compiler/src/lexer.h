#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include "atom.h"
#include "token.h"
#include "utf8.h"
#include "fileutils.h"

typedef struct {
    AtomTable* atom_table;
    const char* path;
    const char* cursor;
    size_t l0, c0;
    const char* end;
    const char* src;
} Lexer;

Lexer lexer_create(const char* ipath, AtomTable* table);
void lexer_cleanup(Lexer* lexer);
Token lexer_next(Lexer* lexer);

Token lexer_peak(Lexer* lexer, size_t ahead);
void lexer_eat(Lexer* lexer, size_t count);
static Token lexer_peak_next(Lexer* lexer) {
    return lexer_peak(lexer, 0);
}
static bool lexer_end(Lexer* lexer) {
    return lexer->cursor == lexer->end || lexer_peak_next(lexer).kind >= TOKEN_END;
}
