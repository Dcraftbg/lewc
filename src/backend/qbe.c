#include "qbe.h"
#include "../token.h"
#include "../darray.h"
#include <errno.h>
typedef struct {
    Atom* name; 
    size_t unnamed_i;
    enum {
        GLOBAL_ARRAY
    } kind;
    struct { Type* type; const void* data; size_t len; } array;
} QbeGlobal;
typedef struct {
    QbeGlobal* items;
    size_t len, cap;
    size_t unnamed_i;
} QbeGlobals;
typedef struct {
    Build* build;
    Module* module;
    FILE* f;
    QbeGlobals globals;
    Arena* arena;
    size_t inst;
} Qbe;
#define nprintf(...) do { \
       if(fprintf(qbe->f, __VA_ARGS__) < 0) {\
           eprintfln("ERROR Failed to encode something");\
           return false;\
       }\
   } while(0)
#define nprintfln(...) do {\
       nprintf(__VA_ARGS__);\
       fputs(NEWLINE, qbe->f);\
   } while(0)
static void alloca_params(size_t type_sz, size_t *sz, size_t *count) {
    *count = type_sz; 
    if(type_sz <= 4) {
        *sz = 4;
    } else if (type_sz <= 8) {
        *sz = 8;
    } else {
        *sz = 16;
    }
}
const char* type_to_qbe_full(Arena* arena, Type* t) {
    assert(t);
    assert(t->core != CORE_FUNC);
    switch(t->core) {
    case CORE_PTR:
        return "l";
    case CORE_BOOL:
    case CORE_I8:
        return aprintf(arena, "%cb", t->unsign ? 'u' : 's');
    case CORE_I16:
        return aprintf(arena, "%ch", t->unsign ? 'u' : 's');
    case CORE_I32:
        return aprintf(arena, "%cw", t->unsign ? 'u' : 's');
    case CORE_STRUCT:
        if(!t->name) todo("Sorry. Anonymous structures aren't supported by QBE yet");
        return aprintf(arena, ":%s", t->name);
    default:
        unreachable("t->core=%d", t->core);
    }
}

const char* type_to_qbe(Arena* arena, Type* t) {
    assert(t);
    assert(t->core != CORE_FUNC);
    switch(t->core) {
    case CORE_PTR:
        return "l";
    case CORE_BOOL:
    case CORE_I8:
    case CORE_I16:
    case CORE_I32:
        return "w";
    case CORE_STRUCT:
        if(!t->name) todo("Sorry. Anonymous structures aren't supported by QBE yet");
        return aprintf(arena, ":%s", t->name);
    default:
        unreachable("t->core=%d", t->core);
    }
}

size_t build_qbe_ast(Qbe* qbe, AST* ast);
size_t build_qbe_deref(Qbe* qbe, size_t what, Type* type) {
    size_t n = 0;
    if(type->core == CORE_STRUCT) {
        nprintfln("    # Deref on structure. Does nothing");
        n = what;
    } else nprintfln("    %%.s%zu =%s load%s %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, type), type_to_qbe_full(qbe->arena, type), what);
    return n;
}
// For assignments. i.e.:
//    foo = 69
//    *bar = 420
size_t build_ptr_to(Qbe* qbe, AST* ast) {
    size_t n = 0;
    switch(ast->kind) {
    case AST_SYMBOL:
        nprintfln("    %%.s%zu =l copy %%%s", n=qbe->inst++, ast->as.symbol.name->data);
        return n;
    case AST_UNARY: 
        if(ast->as.unary.op == '*') return build_qbe_ast(qbe, ast->as.unary.rhs);
        else unreachable("unary.op = %d", ast->as.unary.op);
    case AST_BINOP: {
        assert(ast->as.binop.op == '.');
        size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
        Struct* s;
        Type* type = ast->as.binop.lhs->type;
        if(type->core == CORE_PTR && type->inner_type->core == CORE_STRUCT) {
            s = &type->inner_type->struc;
        } else if (type->core == CORE_STRUCT) {
            s = &type->struc;
        } else unreachable("type->core = %d", type->core);
        Member* m = members_get(&s->members, ast->as.binop.rhs->as.symbol.name);
        assert(m);
        nprintfln("    %%.s%zu =l add %%.s%zu, %zu", n=qbe->inst++, v0, m->offset);
        return n;
    }
    case AST_SUBSCRIPT:
        size_t v0 = build_qbe_ast(qbe, ast->as.subscript.what);
        size_t v1 = build_qbe_ast(qbe, ast->as.subscript.with);
        if(!v0 || !v1) return 0;
        assert(ast->as.subscript.what->type->core == CORE_CONST_ARRAY);
        Type* t = ast->as.subscript.what->type->array.of;
        size_t index = qbe->inst++;
        nprintfln("    %%.s%zu =l ext%s %%.s%zu", index, type_to_qbe_full(qbe->arena, ast->as.subscript.with->type), v1);
        size_t offset = qbe->inst++;
        size_t byte_size = type_size(t);
        if(byte_size > 1) nprintfln("    %%.s%zu =l mul %%.s%zu, %zu", offset, index, byte_size);
        else offset = index;
        v1 = offset;
        nprintfln("    %%.s%zu =l add %%.s%zu, %%.s%zu", n=qbe->inst++, v0, v1);
        return n;
    default:
        unreachable("ast->kind = %d", ast->kind);
    }
}
size_t build_qbe_ast(Qbe* qbe, AST* ast) {
    size_t n=0;
    static_assert(AST_KIND_COUNT == 10, "Update build_qbe_ast");
    switch(ast->kind) {
    case AST_FUNC:
        eprintfln("ERROR TBD build function ast?");
        return 0;
    case AST_NULL:
        nprintfln("    %%.s%zu =l copy 0", n=qbe->inst++);
        break;
    case AST_CAST: {
        size_t v0 = build_qbe_ast(qbe, ast->as.cast.what);
        size_t v0_t_size = type_size(ast->as.cast.what->type);
        size_t into_size = type_size(ast->as.cast.into);
        if(into_size < v0_t_size) {
            nprintfln("    %%.s%zu =%s copy %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.cast.into), v0);
        } else if(into_size > v0_t_size) {
            nprintfln("    %%.s%zu =%s ext%s %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.cast.into), type_to_qbe_full(qbe->arena, ast->as.cast.what->type), v0);
        } else n = v0;
    } break;
    case AST_SUBSCRIPT: {
        size_t v0 = build_qbe_ast(qbe, ast->as.subscript.what);
        size_t v1 = build_qbe_ast(qbe, ast->as.subscript.with);
        if(!v0 || !v1) return 0;
        assert(ast->as.subscript.what->type->core == CORE_CONST_ARRAY);
        Type* t = ast->as.subscript.what->type->array.of;
        size_t index = qbe->inst++;
        nprintfln("    %%.s%zu =l ext%s %%.s%zu", index, type_to_qbe_full(qbe->arena, ast->as.subscript.with->type), v1);
        size_t offset = qbe->inst++;
        size_t byte_size = type_size(t);
        if(byte_size > 1) nprintfln("    %%.s%zu =l mul %%.s%zu, %zu", offset, index, byte_size);
        else offset = index;
        v1 = offset;
        nprintfln("    %%.s%zu =l add %%.s%zu, %%.s%zu", n=qbe->inst++, v0, v1);
        n = build_qbe_deref(qbe, n, t);
    } break;
    case AST_BINOP:
        switch(ast->as.binop.op) {
        // TODO: In conditions this kind of stuff must be evalated as jumps
        // so that order of operation is kept
        case TOKEN_BOOL_AND: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =w and %%.s%zu, %%.s%zu", n=qbe->inst++, v1, v0);
        } break;
        case TOKEN_BOOL_OR: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =w or %%.s%zu, %%.s%zu", n=qbe->inst++, v1, v0);
        } break;

        case '=': {
            size_t v0 = build_ptr_to(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            if(ast->as.binop.lhs->type->core == CORE_STRUCT) {
                // NOTE: blit might not always be supported by QBE. Maybe call qbe -v first?
                nprintfln("    # If your QBE doesn't have blit. Update to the latest version");
                nprintfln("    blit %%.s%zu, %%.s%zu, %zu", v1, v0, type_size(ast->as.binop.lhs->type));
            } else {
                nprintfln("    store%s %%.s%zu, %%.s%zu", type_to_qbe(qbe->arena, ast->type), v1, v0); 
            }
            n = v1;
        } break;
        case TOKEN_SHL: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s shl %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case TOKEN_SHR: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s shr %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '%': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s rem %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '|': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s or %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '^': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s xor %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '*': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s mul %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        // TODO: udiv for unsigned
        case '/': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s div %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '&': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s and %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '-': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintfln("    %%.s%zu =%s sub %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;
        case '+': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            // TODO: Is integer instead
            if(ast->as.binop.lhs->type->core == CORE_PTR && type_isint(ast->as.binop.rhs->type)) {
                size_t index = qbe->inst++;
                nprintfln("    %%.s%zu =l ext%s %%.s%zu", index, type_to_qbe_full(qbe->arena, ast->as.binop.rhs->type), v1);
                size_t offset = qbe->inst++;
                Type* type = ast->as.binop.lhs->type->ptr_count == 1 ? ast->as.binop.lhs->type->inner_type : type_ptr(qbe->arena, ast->as.binop.lhs->type->inner_type, ast->as.binop.lhs->type->ptr_count - 1);
                size_t byte_size = type_size(type);
                if(byte_size > 1) nprintfln("    %%.s%zu =l mul %%.s%zu, %zu", offset, index, byte_size);
                else offset = index;
                v1 = offset;
            }
            nprintfln("    %%.s%zu =%s add %%.s%zu, %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->as.binop.lhs->type), v0, v1);
        } break;

        case '<':
        case TOKEN_LTEQ:
        case '>':
        case TOKEN_GTEQ:
        {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            const char* what = NULL;
            switch(ast->as.binop.op) {
            case '<':
                what = "lt";
                break;
            case TOKEN_LTEQ:
                what = "le";
                break;
            case '>':
                what = "gt";
                break;
            case TOKEN_GTEQ:
                what = "ge";
                break;
            default:
                unreachable("binop = %c", ast->as.binop.op);
            }
            const char* typestr = type_to_qbe(qbe->arena, ast->as.binop.lhs->type);
            nprintfln("    %%.s%zu =%s c%c%s%s %%.s%zu, %%.s%zu", n=qbe->inst++, typestr, ast->as.binop.lhs->type->unsign ? 'u' : 's', what, typestr, v0, v1);
        } break;
        case TOKEN_EQEQ:
        case TOKEN_NEQ:
        {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            const char* what = NULL;
            switch(ast->as.binop.op) {
            case TOKEN_EQEQ:
                what = "eq";
                break;
            case TOKEN_NEQ:
                what = "ne";
                break;
            default:
                unreachable("binop = %c", ast->as.binop.op);
            }
            const char* typestr = type_to_qbe(qbe->arena, ast->as.binop.lhs->type);
            nprintfln("    %%.s%zu =%s c%s%s %%.s%zu, %%.s%zu", n=qbe->inst++, typestr, what, typestr, v0, v1);
        } break;
        case '.': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            Type* t = ast->as.binop.lhs->type;
            assert(t->core == CORE_STRUCT || t->core == CORE_CONST_ARRAY || t->core == CORE_PTR);
            // FIXME: Very very very very dumb patch.
            if(t->core == CORE_PTR) t = t->inner_type;
            switch(t->core) {
            case CORE_STRUCT: {
                Struct* s = &t->struc;
                Member* m = members_get(&s->members, ast->as.binop.rhs->as.symbol.name);
                assert(m);
                if(m->kind == MEMBER_FIELD) {
                    nprintfln("    %%.s%zu =l add %%.s%zu, %zu", n=qbe->inst++, v0, m->offset);
                    if(m->type->core != CORE_STRUCT) {
                        size_t ptr = n;
                        nprintfln("    %%.s%zu =%s load%s %%.s%zu", n=qbe->inst++, type_to_qbe(qbe->arena, m->type), type_to_qbe_full(qbe->arena, m->type), ptr);
                    }
                }
            } break;
            case CORE_CONST_ARRAY: {
                assert(strcmp(ast->as.binop.rhs->as.symbol.name->data, "data") == 0);
                return v0;
            } break;
            }
        } break;
        default:
            unreachable("ast->as.binop.op=%d", ast->as.binop.op);
        }
        break;
    case AST_CALL: {
        assert(ast->as.call.what->kind == AST_SYMBOL);
        struct {
            size_t *items;
            size_t len, cap;
        } result_args = {0};
        CallArgs* args = &ast->as.call.args;
        for(size_t i = 0; i < args->len; ++i) {
            size_t v = build_qbe_ast(qbe, args->items[i]);
            if(!v) return 0;
            da_push(&result_args, v);
        }

        nprintf("    ");
        if(ast->type) {
            nprintf("%%.s%zu =%s ", n=qbe->inst++, type_to_qbe(qbe->arena, ast->type));
        }
        nprintf("call $%s(", ast->as.call.what->as.symbol.name->data);
        for(size_t i = 0; i < args->len; ++i) {
            if(i > 0) nprintf(", ");
            nprintf("%s %%.s%zu", type_to_qbe(qbe->arena, args->items[i]->type), result_args.items[i]);
        }
        nprintfln(")");
    } break;
    case AST_C_STR: {
        QbeGlobal global = {0};
        global.name = NULL;
        global.unnamed_i = qbe->globals.unnamed_i++;
        global.kind = GLOBAL_ARRAY;
        global.array.type = &type_u8;
        global.array.data = ast->as.str.str;
        global.array.len  = ast->as.str.len+1;
        da_push(&qbe->globals, global);
        nprintf("    %%.s%zu =", n=qbe->inst++);nprintfln("l copy $.g%zu", global.unnamed_i);
    } break;
    case AST_UNARY: {
        switch(ast->as.unary.op) {
        case '*': {
            size_t v0 = build_qbe_ast(qbe, ast->as.unary.rhs);
            n = build_qbe_deref(qbe, v0, ast->type);
        } break;
        case '&': {
            n = build_ptr_to(qbe, ast->as.unary.rhs);
        } break;
        default:
            unreachable("unary.op=%c", ast->as.unary.op);
        }
    } break;
    case AST_INT: 
        nprintfln("    %%.s%zu =%s copy %lu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->type), ast->as.integer.value);
        break;
    case AST_SYMBOL: {
        Symbol* s = ast->as.symbol.sym;
        switch(s->kind) {
        case SYMBOL_VARIABLE:
            switch(ast->type->core) {
            case CORE_STRUCT:
            case CORE_CONST_ARRAY:
                nprintfln("    %%.s%zu =l copy %%%s", n=qbe->inst++, ast->as.symbol.name->data);
                break;
            default:
                nprintfln("    %%.s%zu =%s load%s %%%s", n=qbe->inst++, type_to_qbe(qbe->arena, ast->type), type_to_qbe_full(qbe->arena, ast->type), ast->as.symbol.name->data);
                break;
            }
            break;
        case SYMBOL_CONSTANT: {
            AST* value = s->ast;
            switch(value->kind) {
            case AST_INT:
                nprintfln("    %%.s%zu =%s copy %lu", n=qbe->inst++, type_to_qbe(qbe->arena, ast->type), value->as.integer.value);
                break;
            default:
                unreachable("value->kind = %d", value->kind);
            }
        } break;
        default:
            unreachable("s->kind = %d", s->kind);
        }
    } break;
    default:
        eprintfln("ERROR Unsupported. I guess :( %d (build_qbe_ast)", ast->kind);
        exit(1);
        break;
    }
    return n;
}
bool build_qbe_cond(Qbe* qbe, AST* ast, const char* yes, const char* no) {
    if(ast->kind == AST_BINOP) {
        switch(ast->as.binop.op) {
        case TOKEN_BOOL_AND: {
            const char* and_rhs = aprintf(qbe->arena, "@and_rhs_%zu", qbe->inst++);
            build_qbe_cond(qbe, ast->as.binop.lhs, and_rhs, no);
            nprintfln("%s", and_rhs);
            build_qbe_cond(qbe, ast->as.binop.rhs, yes, no);
            return true;
        }
        case TOKEN_BOOL_OR:
            const char* or_rhs = aprintf(qbe->arena, "@or_rhs_%zu", qbe->inst++);
            build_qbe_cond(qbe, ast->as.binop.lhs, yes, or_rhs);
            nprintfln("%s", or_rhs);
            build_qbe_cond(qbe, ast->as.binop.rhs, yes, no);
            return true;
        }
    }
    size_t cond = build_qbe_ast(qbe, ast);
    nprintfln("    jnz %%.s%zu, %s, %s", cond, yes, no);
    return true;
}
bool build_qbe_scope(Qbe* qbe, Statements* scope);
bool build_qbe_statement(Qbe* qbe, Statement* statement) {
    size_t n = 0;
    static_assert(STATEMENT_COUNT == 8, "Update build_qbe_statement");
    switch(statement->kind) {
    case STATEMENT_EVAL:
        build_qbe_ast(qbe, statement->as.ast);
        break;
    case STATEMENT_RETURN:
        if(statement->as.ast) {
            n = build_qbe_ast(qbe, statement->as.ast);
            nprintfln("    ret %%.s%zu", n);
        } else {
            nprintfln("    ret");
        }
        break;
    case STATEMENT_LOCAL_DEF: {
        Symbol* s = statement->as.local_def.symbol;
        Type* type = s->type;
        Atom* name = statement->as.local_def.name;
        AST * init = s->ast;
        size_t sz, count;
        alloca_params(type_size(type), &sz, &count);
        nprintfln("    %%%s =l alloc%zu %zu", name->data, sz, count);
        if(init) {
            size_t v1 = build_qbe_ast(qbe, init);
            if(type->core == CORE_STRUCT) {
                // NOTE: blit might not always be supported by QBE. Maybe call qbe -v first?
                nprintfln("    # If your QBE doesn't have blit. Update to the latest version");
                nprintfln("    blit %%.s%zu, %%%s, %zu", v1, name->data, type_size(type));
            } else {
                nprintfln("    store%s %%.s%zu, %%%s", type_to_qbe(qbe->arena, type), v1, name->data); 
            }
        }
    } break;
    case STATEMENT_SCOPE:
        if(!build_qbe_scope(qbe, statement->as.scope)) return false;
        break;
    case STATEMENT_LOOP: {
        n = qbe->inst;
        nprintfln("@loop%zu", n);
        if(!build_qbe_statement(qbe, statement->as.loop.body)) return false;
        if(!statement->as.loop.body->terminal) {
            nprintfln("    jmp @loop%zu", n);
        }
        nprintfln("@loop_end_%zu", n);
    } break;
    case STATEMENT_IF: {
        n = qbe->inst++;
        const char* cond = aprintf(qbe->arena, "@if_cond_%zu", n);
        const char* body = aprintf(qbe->arena, "@if_body_%zu", n);
        const char* end  = aprintf(qbe->arena, "@if_end_%zu", n);
        const char* elze = statement->as.iff.elze ? aprintf(qbe->arena, "@if_elze_%zu", n) : end; 
        nprintfln("%s", cond);
        build_qbe_cond(qbe, statement->as.iff.cond, body, elze);
        nprintfln("%s", body);
        if(!build_qbe_statement(qbe, statement->as.iff.body)) return false;
        if(!statement->as.iff.body->terminal) {
            nprintfln("    jmp %s", end);
        }
        if(statement->as.iff.elze) {
            nprintfln("%s", elze);
            if(!build_qbe_statement(qbe, statement->as.iff.elze)) return false;
            // NOTE: Here its not necessary cuz QBE is cool and has automatic fallthrough
        }
        nprintfln("%s", end);
    } break;
    case STATEMENT_WHILE: {
        n = qbe->inst++;
        const char* cond = aprintf(qbe->arena, "@while_cond_%zu", n);
        const char* body = aprintf(qbe->arena, "@while_body_%zu", n);
        const char* end  = aprintf(qbe->arena, "@while_end_%zu", n);
        nprintfln("%s", cond);
        build_qbe_cond(qbe, statement->as.whil.cond, body, end);
        nprintfln("%s", body);
        if(!build_qbe_statement(qbe, statement->as.whil.body)) return false;
        if(!statement->as.whil.body->terminal) {
            nprintfln("    jmp %s", cond);
        }
        nprintfln("%s", end);
    } break;
    // defer is a makeshift statement. Its here just to indicate its existence for 
    // other processes throughout the chain to verify
    case STATEMENT_DEFER: 
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool build_qbe_scope(Qbe* qbe, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        if(!build_qbe_statement(qbe, scope->items[j])) return false;
    }
    return true;
}
bool build_qbe_qbe(Qbe* qbe) {
    nprintfln("# Generated by lewc compiler");
    nprintfln("# Target: %s %s", platform_str(qbe->build->target->platform), arch_str(qbe->build->target->arch));
    // TODO: Fuck me, QBE requires types to be in order for some fucking reason.
    // And also no implicit types I guess
    for(size_t i = 0; i < qbe->module->type_table.buckets.len; ++i) {
        for(
            Pair_TypeTable* tpair = qbe->module->type_table.buckets.items[i].first;
            tpair;
            tpair = tpair->next
        ) {
            Type* t = tpair->value;
            const char* name = tpair->key;
            if(t->core != CORE_STRUCT && t->core != CORE_CONST_ARRAY) continue;
            nprintfln("type :%s = { b %zu }", name, type_size(t));
        }
    }
    for(size_t i = 0; i < qbe->module->symbols.len; ++i) {
        Symbol* s  = qbe->module->symbols.items[i].symbol;
        Atom* name = qbe->module->symbols.items[i].name;
        qbe->inst = 1;
        if(s->ast->kind != AST_FUNC) continue;
        Function* func = s->ast->as.func;
        assert(func->type->core == CORE_FUNC);
        // I think QBE automatically assumes external function
        if(func->type->attribs & TYPE_ATTRIB_EXTERN) {
            nprintf("# extern %s :: ", name->data);
            type_dump(qbe->f, func->type);
            nprintf("%s", NEWLINE);
            continue;
        }
        nprintf("# %s :: ", name->data);
        type_dump(qbe->f, func->type);
        nprintf("%s", NEWLINE);
        nprintf("export function");
        if(func->type->signature.output) {
            nprintf(" %s", type_to_qbe(qbe->arena, func->type->signature.output));
        }
        nprintf(" $%s (", name->data);
        for(size_t i = 0; i < func->type->signature.input.len; ++i) {
            if(i > 0) nprintf(", ");
            Arg* arg = &func->type->signature.input.items[i];
            
            nprintf("%s", type_to_qbe(qbe->arena, arg->type));
            if(arg->name && arg->type->core == CORE_STRUCT) {
                nprintf(" %%%s", arg->name->data);
            } else nprintf(" %%.a%zu", i);
        }
        nprintfln(") {");
        nprintfln("@start");
        for(size_t i = 0; i < func->type->signature.input.len; ++i) {
            Arg* arg = &func->type->signature.input.items[i];
            if(arg->type->core == CORE_STRUCT) continue;
            size_t sz, count;
            alloca_params(type_size(arg->type), &sz, &count);
            if(arg->name) {
                nprintfln("    %%%s =l alloc%zu %zu", arg->name->data, sz, count);
                if(arg->type->core == CORE_BOOL) {
                    size_t n;
                    nprintfln("    %%.s%zu =w cnew %%.a%zu, 0", (n=qbe->inst++), i);
                    nprintfln("    store%s %%.s%zu, %%%s", type_to_qbe(qbe->arena, arg->type), n, arg->name->data);
                } else {
                    nprintfln("    store%s %%.a%zu, %%%s", type_to_qbe(qbe->arena, arg->type), i, arg->name->data);
                }
            }
        }
        if(!build_qbe_scope(qbe, func->scope)) return false;
        nprintfln("}");
        
    }
    for(size_t i = 0; i < qbe->globals.len; ++i) {
        QbeGlobal* global = &qbe->globals.items[i];
        assert(global->kind == GLOBAL_ARRAY);
        assert(global->array.type->core == CORE_I8);
        assert(global->name == NULL);
        nprintf("data $.g%zu = {", global->unnamed_i);
        for(size_t j = 0; j < global->array.len; ++j) {
            if(j > 0) nprintf(", ");
            nprintf("b %d", ((uint8_t*)global->array.data)[j]);
        }
        nprintfln(" }");
    }
    return true;
}
bool build_qbe(Build* build, ProgramState* state) {
    // TODO: Its more correct to call it SysV
    assert(build->target->platform == OS_LINUX);
    assert(build->target->arch == ARCH_X86_64);
    assert(build->target->outputKind == OUTPUT_GAS);
    Qbe qbe = {
        .build = build,
        .module = state->main,
        .f = NULL,
        .arena = state->arena,
    };
    char ssa_path[1024];
    size_t opath_len = strlen(build->options->opath);
    assert((opath_len + 4 + 1) <= sizeof(ssa_path));
    memcpy(ssa_path, build->options->opath, opath_len);
    strcpy(ssa_path+opath_len, ".ssa");
    qbe.f = fopen(ssa_path, "wb");
    if(!qbe.f) {
        eprintfln("ERROR Could not open output file %s: %s", ssa_path, strerror(errno));
        return false;
    }
    if(!build_qbe_qbe(&qbe)) {
        fclose(qbe.f);
        return false;
    }
    fclose(qbe.f);
    // More than enough for everyone
    char* cmd = malloc(4096);
    if(!cmd) {
        eprintfln("ERROR Not enough memory for cmd");
        return false;
    }
    strcpy(cmd, "qbe ");
    strcat(cmd, ssa_path);
    strcat(cmd, " -o ");
    strcat(cmd, build->options->opath);
    int e = system(cmd);
    free(cmd);
    if(e != 0) {
        eprintfln("ERROR Failed to build with qbe");
        return false;
    }
    return true;
}
