#include "lexer.h"

void lexer_create(Lexer* lexer, const char* ipath, AtomTable* table, Arena* arena) {
    size_t size = 0;
    lexer->src = read_entire_file(ipath, &size);
    // TODO: error handling or something?
    if(!lexer->src) {
        eprintfln("ERROR: Could not read source file: %s",ipath);
        exit(1);
    }
    lexer->cursor = lexer->src;
    lexer->end = lexer->src+size;
    lexer->l0 = 1;
    lexer->c0 = 0;
    lexer->atom_table = table;
    lexer->arena = arena;
    lexer->path = ipath;
    scratchbuf_init(&lexer->buf);
}
static uint32_t lexer_peak_c_n(Lexer* lexer, size_t n) {
    const char* strm = lexer->cursor;
    uint32_t c;
    do {
        c = utf8_next(&strm, lexer->end);
    } while(n--);
    return c;
}
static uint32_t lexer_peak_c(Lexer* lexer) {
    return lexer_peak_c_n(lexer, 0);
}
static uint32_t lexer_next_c(Lexer* lexer) {
    uint32_t res = utf8_next(&lexer->cursor, lexer->end);
    lexer->c0++;
    if (res == '\n') {
        lexer->l0++;
        lexer->c0 = 0;
    }
    return res;
}
void lexer_cleanup(Lexer* lexer) {
    FS_DEALLOC((void*)lexer->src, (size_t)(lexer->end-lexer->src));
}
static void lexer_trim(Lexer* lexer) {
    while (lexer->cursor < lexer->end && isspace(lexer_peak_c(lexer))) lexer_next_c(lexer);
}
typedef struct {
    const char *start, *end;
} Word;
static bool iswordc(uint32_t codepoint) {
    return codepoint == '_' || isalnum(codepoint);
}
static size_t wordlen(Word word) {
    return (size_t)(word.end-word.start);
}
static bool wordeq(Word word, const char* cstr) {
    size_t len = strlen(cstr);
    return len == wordlen(word) && memcmp(word.start, cstr, len) == 0;
}
static Word lexer_parse_word(Lexer* lexer) {
    const char* start = lexer->cursor;
    while (lexer->cursor < lexer->end && iswordc(lexer_peak_c(lexer))) lexer_next_c(lexer);
    return (Word){start,lexer->cursor};
}
static void lex_str(Lexer* lexer, const char **str, size_t *len) {
    int c;
    bool escape=false;
    if((c=lexer_next_c(lexer)) != '"') {
        eprintfln("ERROR: %s:%zu:%zu: Expected '\"' at the start of string but found %c", lexer->path, lexer->l0, lexer->c0, c);
        *str = NULL;
        return;
    }
    while (lexer->cursor < lexer->end && (escape || (lexer_peak_c(lexer) != '"'))) {
        c = lexer_next_c(lexer);
        if(escape) {
            switch(c) {
            case 't':
                scratchbuf_push(&lexer->buf, '\t');
                break;
            case 'n':
                scratchbuf_push(&lexer->buf, '\n');
                break;
            case 'r':
                scratchbuf_push(&lexer->buf, '\r');
                break;
            case '0':
                scratchbuf_push(&lexer->buf, '\0');
                break;
            default:
                if(c >= 256) {
                     eprintfln("ERROR: %s:%zu:%zu: UTF8 characters are not yet supported in strings :(", lexer->path, lexer->l0, lexer->c0);
                     abort();
                }
                scratchbuf_push(&lexer->buf, c);
                break;
            }
            escape = false;
        } else {
            if(c == '\\') {
                escape = true;
            }
            else {
                if(c >= 256) {
                     eprintfln("ERROR: %s:%zu:%zu: UTF8 characters are not yet supported in strings :(", lexer->path, lexer->l0, lexer->c0);
                     abort();
                }
                scratchbuf_push(&lexer->buf, c);
            }
        }
    }
    if((c=lexer_next_c(lexer)) != '"') {
        eprintfln("ERROR: %s:%zu:%zu: Expected '\"' at the end of string but found %c", lexer->path, lexer->l0, lexer->c0, c);
        *str = NULL;
        return;
    }
    *len = lexer->buf.len;
    scratchbuf_push(&lexer->buf, '\0');
    void* buf = arena_alloc(lexer->arena, lexer->buf.len);
    assert(buf);
    memcpy(buf, lexer->buf.data, lexer->buf.len);
    *str = buf;
    scratchbuf_reset(&lexer->buf);
    return;
}
#define MAKE_TOKEN(...) (Token) { lexer->path, l0, c0, lexer->l0, lexer->c0, .kind=__VA_ARGS__ }
static size_t lex_prefix_to_radix(Lexer* lexer) {
    int c = lexer_peak_c(lexer);
    switch(c) {
    case 'x': lexer_next_c(lexer); return 16;
    case 'o': lexer_next_c(lexer); return 8;
    case 'b': lexer_next_c(lexer); return 2;
    case '0': return 0;
    }
    return 10;
}
#define is_base16 isxdigit
static Token lex_num_base16(size_t l0, size_t c0, Lexer* lexer) {
    uint32_t c;
    uint64_t result=0;
    while(lexer->cursor < lexer->end && (is_base16((c=lexer_peak_c(lexer))) || c == '_')) {
        c = lexer_next_c(lexer);
        if(c >= '0' && c <= '9') {
            result = result * 16 + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result = result * 16 + 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            result = result * 16 + 10 + (c - 'A');
        } else if (c == '_') {}
        else {
            eprintfln("ERROR:%s:%zu:%zu: Invalid character in base16 integer '%c' (%u)", lexer->path, lexer->l0, lexer->c0, c, c);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
    }
    return MAKE_TOKEN(TOKEN_INT, .integer={ .value = result });
}
static bool is_base2(uint32_t c) {
    return c >= '0' && c <= '1';
}
static Token lex_num_base2(size_t l0, size_t c0, Lexer* lexer) {
    uint32_t c;
    uint64_t result=0;
    while(lexer->cursor < lexer->end && (is_base2((c=lexer_peak_c(lexer))) || c == '_')) {
        c = lexer_next_c(lexer);
        if(is_base2(c)) {
            result = result * 2 + (c - '0');
        } else if (c == '_') {}
        else {
            eprintfln("ERROR:%s:%zu:%zu: Invalid character in base10 integer '%c' (%u)", lexer->path, lexer->l0, lexer->c0, c, c);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
    }
    return MAKE_TOKEN(TOKEN_INT, .integer={ .value = result });
}
#define is_base10 isdigit
static Token lex_num_base10(size_t l0, size_t c0, Lexer* lexer) {
    uint32_t c;
    uint64_t result=0;
    while(lexer->cursor < lexer->end && (is_base10((c=lexer_peak_c(lexer))) || c == '_')) {
        c = lexer_next_c(lexer);
        if(is_base10(c)) {
            result = result * 10 + (c - '0');
        } else if (c == '_') {}
        else {
            eprintfln("ERROR:%s:%zu:%zu: Invalid character in base10 integer '%c' (%u)", lexer->path, lexer->l0, lexer->c0, c, c);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
    }
    return MAKE_TOKEN(TOKEN_INT, .integer={ .value = result });
}
static bool is_base8(uint32_t c) {
    return c >= '0' && c <= '7';
}
static Token lex_num_base8(size_t l0, size_t c0, Lexer* lexer) {
    uint32_t c;
    uint64_t result=0;
    while(lexer->cursor < lexer->end && (is_base8((c=lexer_peak_c(lexer))) || c == '_')) {
        c = lexer_next_c(lexer);
        if (is_base8(c)) {
            result = result * 8 + (c - '0');
        } else if (c == '_') {}
        else {
            eprintfln("ERROR:%s:%zu:%zu: Invalid character in base10 integer '%c' (%u)", lexer->path, lexer->l0, lexer->c0, c, c);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
    }
    return MAKE_TOKEN(TOKEN_INT, .integer={ .value = result });
}
static Token lex_num_by_radix(size_t l0, size_t c0, Lexer* lexer, size_t radix) {
    switch(radix) {
    case 10: return lex_num_base10(l0, c0, lexer);
    case 16: return lex_num_base16(l0, c0, lexer);
    case 2 : return lex_num_base2 (l0, c0, lexer);
    case 8 : return lex_num_base8(l0, c0, lexer);
    default:
        eprintfln("ERROR:%s:%zu:%zu: Unsupported integer with radix=%zu", lexer->path, l0, c0, radix);
        return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
    }
    return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
}
#include "type.h"
// TODO: Check for overflow here maybe?
static bool lex_num_suffix(Lexer* lexer, Token* t) {
    Word word = lexer_parse_word(lexer);
    if(word.start == word.end) return true;
    if (wordeq(word, "u8")) {
        t->integer.type = &type_u8;
    } else if (wordeq(word, "u16")) {
        t->integer.type = &type_u16;
    } else if (wordeq(word, "bool")) {
        t->integer.type = &type_bool;
    } else if (wordeq(word, "i32")) {
        t->integer.type = &type_i32;
    } else {
        eprintfln("ERROR:%s: Invalid suffix `%.*s`", tloc(*t), (int)(word.end-word.start), word.start);
        return false;
    }
    return true;
}
Token lexer_next(Lexer* lexer) {
    lexer_trim(lexer);
    size_t l0 = lexer->l0;
    size_t c0 = lexer->c0;
    if (lexer->cursor >= lexer->end) return MAKE_TOKEN(TOKEN_EOF);
    uint32_t c = lexer_peak_c(lexer);
    switch (c) {
    case '(':
    case ')':
    case '{':
    case '}':
    case ',':
    case '+':
    case '|':
    case '^':
    case ';':    
    case ':':
    case '&':
    case '*':
    case '%':
        lexer_next_c(lexer);
        return MAKE_TOKEN(c);
    case '<':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '<') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_SHL);
        }
        if(lexer_peak_c(lexer) == '=') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_LTEQ);
        }
        return MAKE_TOKEN(c);
    case '>':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '>') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_SHR);
        }
        if(lexer_peak_c(lexer) == '=') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_GTEQ);
        }
        return MAKE_TOKEN(c);
    case '!':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '=') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_NEQ);
        }
        return MAKE_TOKEN(c);
    case '=':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '=') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_EQEQ);
        }
        return MAKE_TOKEN(c);
    case '-':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '>') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_ARROW);
        }
        return MAKE_TOKEN(c);
    case '/': 
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '/') {
            lexer_next_c(lexer);
            while (lexer->cursor < lexer->end && lexer_peak_c(lexer) != '\n') lexer_next_c(lexer);
            return lexer_next(lexer);
        }
        return MAKE_TOKEN(c);
    case '0': {
        lexer_next_c(lexer);
        if(lexer->cursor >= lexer->end) return MAKE_TOKEN(TOKEN_INT, .integer= { .value = 0 });
        size_t radix = lex_prefix_to_radix(lexer);
        if(!radix) {
            eprintfln("ERROR:%s:%zu:%zu Cannot have an integer with multiple 0's (i.e. 000000)", lexer->path, lexer->l0, lexer->c0);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
        Token t = lex_num_by_radix(l0, c0, lexer, radix);
        if(!lex_num_suffix(lexer, &t)) {
            eprintfln("ERROR:%s:%zu:%zu Invalid type suffix for integer", lexer->path, lexer->l0, lexer->c0);
            return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
        }
        return t;
    }
    default:
        if (c >= '0' && c <= '9') {
            Token t = lex_num_by_radix(l0, c0, lexer, 10);
            if(!lex_num_suffix(lexer, &t)) {
                eprintfln("ERROR:%s:%zu:%zu Invalid type suffix for integer", lexer->path, lexer->l0, lexer->c0);
                return MAKE_TOKEN(TOKEN_INVALID_INT_LITERAL);
            }
            return t;
        } else if(c == 'c' && lexer_peak_c_n(lexer, 1) == '"') {
            lexer_next_c(lexer);
            const char* str = NULL;
            size_t len=0;
            lex_str(lexer, &str, &len);
            if(!str) return MAKE_TOKEN(TOKEN_INVALID_STR);
            return MAKE_TOKEN(TOKEN_C_STR, .str = str, .str_len = len);
        } else if(c == '_' || isalpha(c)) {
            Word word = lexer_parse_word(lexer);
                 if (wordeq(word, "return" )) return MAKE_TOKEN(TOKEN_RETURN);
            else if (wordeq(word, "extern" )) return MAKE_TOKEN(TOKEN_EXTERN);
            else if (wordeq(word, "while"  )) return MAKE_TOKEN(TOKEN_WHILE);
            else if (wordeq(word, "loop"   )) return MAKE_TOKEN(TOKEN_LOOP);
            else if (wordeq(word, "typedef")) return MAKE_TOKEN(TOKEN_TYPEDEF);
            return MAKE_TOKEN(TOKEN_ATOM, .atom = atom_alloc(lexer->atom_table,word.start, (size_t)(word.end-word.start)));
        }
    }
    return MAKE_TOKEN(TOKEN_UNPARSABLE, .codepoint = c);
}
Snapshot lexer_snap_take(Lexer* lexer) {
    return (Snapshot) {
        .cursor = lexer->cursor,
        .l0 = lexer->l0,
        .c0 = lexer->c0
    };
}
void lexer_snap_restore(Lexer* lexer, Snapshot snap) {
    lexer->cursor = snap.cursor;
    lexer->l0 = snap.l0;
    lexer->c0 = snap.c0;
}

Token lexer_peak(Lexer* lexer, size_t ahead) {
    Snapshot snap = lexer_snap_take(lexer);
    Token t={0};
    do {
        t = lexer_next(lexer);
    } while(ahead--); 
    lexer_snap_restore(lexer, snap);
    return t;
}

void lexer_eat(Lexer* lexer, size_t count) {
    for(size_t i = 0; i < count; ++i) {
        lexer_next(lexer);
    }
}

