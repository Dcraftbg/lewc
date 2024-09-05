#include "lexer.h"

typedef struct {
    const char* cursor;
    size_t l0, c0;
} Snapshot;
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
    case ';':    
    case ':':    
        lexer_next_c(lexer);
        return MAKE_TOKEN(c);
    case '-':
        lexer_next_c(lexer);
        if(lexer_peak_c(lexer) == '>') {
            lexer_next_c(lexer);
            return MAKE_TOKEN(TOKEN_ARROW);
        }
        return MAKE_TOKEN(c);
    default:
        if(c == 'c' && lexer_peak_c_n(lexer, 1) == '"') {
            lexer_next_c(lexer);
            const char* str = NULL;
            size_t len=0;
            lex_str(lexer, &str, &len);
            if(!str) return MAKE_TOKEN(TOKEN_INVALID_STR);
            return MAKE_TOKEN(TOKEN_C_STR, .str = str, .str_len = len);
        } else if(c == '_' || isalpha(c)) {
            Word word = lexer_parse_word(lexer);
                 if (wordeq(word, "return")) return MAKE_TOKEN(TOKEN_RETURN);
            else if (wordeq(word, "extern")) return MAKE_TOKEN(TOKEN_EXTERN);
            return MAKE_TOKEN(TOKEN_ATOM, .atom = atom_alloc(lexer->atom_table,word.start, (size_t)(word.end-word.start)));
        }
    }
    return MAKE_TOKEN(TOKEN_UNPARSABLE, .codepoint = c);
}
static Snapshot lexer_snap_take(Lexer* lexer) {
    static_assert(sizeof(Snapshot) == 24, "Update lexer_snap_take");
    return (Snapshot) {
        .cursor = lexer->cursor,
        .l0 = lexer->l0,
        .c0 = lexer->c0
    };
}
static void lexer_snap_restore(Lexer* lexer, Snapshot snap) {
    static_assert(sizeof(Snapshot) == 24, "Update lexer_snap_restore");
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

