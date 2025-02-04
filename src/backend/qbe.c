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
    ProgramState* state;
    FILE* f;
    QbeGlobals globals;
    size_t inst;
} Qbe;
#define nprintf(...) do { \
       if(fprintf(qbe->f, __VA_ARGS__) < 0) {\
           eprintfln("ERROR: Failed to encode something");\
           return false;\
       }\
   } while(0)
#define nprintfln(...) do {\
       nprintf(__VA_ARGS__);\
       fputs(NEWLINE, qbe->f);\
   } while(0)
bool dump_type_to_qbe_full(Qbe* qbe, Type* t) {
    assert(t);
    assert(t->core != CORE_FUNC);
    switch(t->core) {
    case CORE_PTR:
        nprintf("l");
        break;
    case CORE_I8:
        if(t->unsign) nprintf("u");
        else nprintf("s");
        nprintf("b");
        break;
    case CORE_I16:
        if(t->unsign) nprintf("u");
        else nprintf("s");
        nprintf("h");
        break;
    case CORE_I32:
        if(t->unsign) nprintf("u");
        else nprintf("s");
        nprintf("w");
        break;
    case CORE_STRUCT:
        if(!t->name) todo("Sorry. Anonymous structures aren't supported by QBE yet");
        nprintf(":%s", t->name);
        break;
    default:
        unreachable("t->core=%d", t->core);
    }
    return true;
}
bool dump_type_to_qbe(Qbe* qbe, Type* t) {
    assert(t);
    assert(t->core != CORE_FUNC);
    switch(t->core) {
    case CORE_PTR:
        nprintf("l");
        break;
    case CORE_BOOL:
    case CORE_I8:
    case CORE_I16:
    case CORE_I32:
        nprintf("w");
        break;
    case CORE_STRUCT:
        if(!t->name) todo("Sorry. Anonymous structures aren't supported by QBE yet");
        nprintf(":%s", t->name);
        break;
    default:
        unreachable("t->core=%d", t->core);
    }
    return true;
}

size_t build_qbe_ast(Qbe* qbe, AST* ast);
// For assignments. i.e.:
//    foo = 69
//    *bar = 420
size_t build_ptr_to(Qbe* qbe, AST* ast) {
    size_t n = 0;
    switch(ast->kind) {
    case AST_SYMBOL: {
        nprintfln("    %%.s%zu =l copy %%%s", n=qbe->inst++, ast->as.symbol.name->data);
    } return n;
    case AST_UNARY: 
        if(ast->as.unary.op == '*') return build_qbe_ast(qbe, ast->as.unary.rhs);
        else unreachable("unary.op = %d", ast->as.unary.op);
    default:
        unreachable("ast->kind = %d", ast->kind);
    }
}
size_t build_qbe_ast(Qbe* qbe, AST* ast) {
    size_t n=0;
    static_assert(AST_KIND_COUNT == 7, "Update build_qbe_ast");
    switch(ast->kind) {
    case AST_FUNC:
        eprintfln("ERROR: TBD build function ast?");
        return 0;
    case AST_BINOP:
        switch(ast->as.binop.op) {
        case '=': {
            size_t v0 = build_ptr_to(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    store"); dump_type_to_qbe(qbe, ast->type); nprintfln(" %%.s%zu, %%.s%zu", v1, v0);
            n = v1;
        } break;
        case TOKEN_SHL: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" shl %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case TOKEN_SHR: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" shr %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '%': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" rem %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '|': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" or %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '^': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" xor %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '*': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" mul %%.s%zu, %%.s%zu", v0, v1);
        } break;
        // TODO: udiv for unsigned
        case '/': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" div %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '&': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" and %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '-': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" sub %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '+': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            // TODO: Is integer instead
            if(ast->as.binop.lhs->type->core == CORE_PTR && type_isbinary(ast->as.binop.rhs->type)) {
                size_t index = qbe->inst++;
                nprintf("    %%.s%zu =l", index);nprintf(" ext");dump_type_to_qbe_full(qbe, ast->as.binop.rhs->type);nprintfln(" %%.s%zu", v1);
                size_t offset = qbe->inst++;
                size_t byte_size = 0;
                Type* type = ast->as.binop.lhs->type->inner_type;
                // TODO: factor this out
                switch(type->core) {
                case CORE_BOOL:
                case CORE_I8:
                    byte_size = 1;
                    break;
                case CORE_I16:
                    byte_size = 2;
                    break;
                case CORE_I32:
                    byte_size = 4;
                    break;
                case CORE_PTR:
                    byte_size = 8;
                    break;
                default:
                    unreachable("type->core=%d", type->core);
                }
                if(byte_size > 1) nprintfln("    %%.s%zu =l mul %%.s%zu, %zu", offset, index, byte_size);
                else offset = index;
                v1 = offset;
            }
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" add %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '<': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" c"); nprintf("%c", ast->as.binop.lhs->type->unsign ? 'u' : 's'); nprintf("lt");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case TOKEN_LTEQ: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" c"); nprintf("%c", ast->as.binop.lhs->type->unsign ? 'u' : 's'); nprintf("le");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case '>': {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" c"); nprintf("%c", ast->as.binop.lhs->type->unsign ? 'u' : 's'); nprintf("gt");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case TOKEN_GTEQ: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" c"); nprintf("%c", ast->as.binop.lhs->type->unsign ? 'u' : 's'); nprintf("ge");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case TOKEN_NEQ: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" cne");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
        } break;
        case TOKEN_EQEQ: {
            size_t v0 = build_qbe_ast(qbe, ast->as.binop.lhs);
            size_t v1 = build_qbe_ast(qbe, ast->as.binop.rhs);
            if(!v0 || !v1) return 0;
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintf(" ceq");dump_type_to_qbe(qbe, ast->as.binop.lhs->type);nprintfln(" %%.s%zu, %%.s%zu", v0, v1);
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
        if(ast->type) {
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->type); nprintf(" ");
        } else {
            nprintf("    ");
        }
        nprintf("call $%s(", ast->as.call.what->as.symbol.name->data);
        for(size_t i = 0; i < args->len; ++i) {
            if(i > 0) nprintf(", ");
            dump_type_to_qbe(qbe, args->items[i]->type);nprintf(" %%.s%zu", result_args.items[i]);
        }
        nprintfln(")");
        free(args->items);
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
        size_t v0 = build_qbe_ast(qbe, ast->as.unary.rhs);
        switch(ast->as.unary.op) {
        case '*':
            nprintf("    %%.s%zu =", n=qbe->inst++); dump_type_to_qbe(qbe, ast->type);nprintf(" load");dump_type_to_qbe_full(qbe, ast->type); nprintfln(" %%.s%zu", v0);
            break;
        default:
            unreachable("unary.op=%c", ast->as.unary.op);
        }
    } break;
    case AST_INT: 
        nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->type);nprintfln(" copy %lu", ast->as.integer.value);
        break;
    case AST_SYMBOL: {
        Symbol* s = ast->as.symbol.sym;
        switch(s->kind) {
        case SYMBOL_VARIABLE:
            nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->type);nprintf(" load");dump_type_to_qbe_full(qbe, ast->type);nprintfln(" %%%s", ast->as.symbol.name->data);
            break;
        case SYMBOL_CONSTANT: {
            AST* value = s->ast;
            switch(value->kind) {
            case AST_INT:
                nprintf("    %%.s%zu =", n=qbe->inst++);dump_type_to_qbe(qbe, ast->type);nprintfln(" copy %lu", value->as.integer.value);
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
        eprintfln("ERROR: Unsupported. I guess :( %d (build_qbe_ast)", ast->kind);
        exit(1);
        break;
    }
    return n;
}
static void alloca_params(size_t type_sz, size_t *sz, size_t *count) {
    if(type_sz <= 4) {
        *sz = 4;
        *count = (type_sz + 3) / 4;
    } else if (type_sz <= 8) {
        *sz = 8;
        *count = (type_sz + 7) / 8;
    } else {
        *sz = 16;
        *count = (type_sz + 15) / 16;
    }
}
bool build_qbe_scope(Qbe* qbe, Statements* scope);
bool build_qbe_statement(Qbe* qbe, Statement* statement) {
    size_t n = 0;
    static_assert(STATEMENT_COUNT == 6, "Update build_qbe_statement");
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
            size_t n = build_qbe_ast(qbe, init);
            nprintf("    store");dump_type_to_qbe(qbe, type);nprintfln(" %%.s%zu, %%%s", n, name->data);
        }
    } break;
    case STATEMENT_SCOPE:
        if(!build_qbe_scope(qbe, statement->as.scope)) return false;
        break;
    case STATEMENT_LOOP: {
        n = qbe->inst;
        nprintfln("@loop%zu", n);
        if(!build_qbe_statement(qbe, statement->as.loop.body)) return false;
        nprintfln("    jmp @loop%zu", n);
        nprintfln("@loop_end_%zu", n);
    } break;
    case STATEMENT_WHILE: {
        n = qbe->inst;
        nprintfln("@while_cond_%zu", n);
        size_t cond = build_qbe_ast(qbe, statement->as.whil.cond);
        nprintfln("    jnz %%.s%zu, @while_body_%zu, @while_end_%zu", cond, n, n);
        nprintfln("@while_body_%zu", n);
        if(!build_qbe_statement(qbe, statement->as.whil.body)) return false;
        nprintfln("    jmp @while_cond_%zu", n);
        nprintfln("@while_end_%zu", n);
    } break;
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
    for(size_t i = 0; i < qbe->state->type_table.buckets.len; ++i) {
        for(
            Pair_TypeTable* tpair = qbe->state->type_table.buckets.items[i].first;
            tpair;
            tpair = tpair->next
        ) {
            Type* t = tpair->value;
            const char* name = tpair->key;
            if(t->core != CORE_STRUCT) continue;
            nprintfln("type :%s = { b %zu }", name, type_size(t));
        }
    }
    for(size_t i = 0; i < qbe->state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = qbe->state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            qbe->inst = 1;
            Symbol* s = spair->value;
            if(s->ast->kind != AST_FUNC) continue;
            Atom* name     = spair->key;
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
            nprintf("export function ");
            if(func->type->signature.output) dump_type_to_qbe(qbe, func->type->signature.output);
            nprintf(" $%s (", name->data);
            for(size_t i = 0; i < func->type->signature.input.len; ++i) {
                if(i > 0) nprintf(", ");
                Arg* arg = &func->type->signature.input.items[i];
                dump_type_to_qbe(qbe, arg->type);
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
                    // nprintf("    store");dump_type_to_qbe(qbe, arg->type);nprintfln(" %%.a%zu, %%%s", i, arg->name->data);
                }
            }
            if(!build_qbe_scope(qbe, func->scope)) return false;
            nprintfln("}");
        }
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
        .state = state,
        .f = NULL
    };
    char ssa_path[1024];
    size_t opath_len = strlen(build->options->opath);
    assert((opath_len + 4 + 1) <= sizeof(ssa_path));
    memcpy(ssa_path, build->options->opath, opath_len);
    strcpy(ssa_path+opath_len, ".ssa");
    qbe.f = fopen(ssa_path, "wb");
    if(!qbe.f) {
        eprintfln("ERROR: Could not open output file %s: %s", ssa_path, strerror(errno));
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
        eprintfln("ERROR: Not enough memory for cmd");
        return false;
    }
    strcpy(cmd, "qbe ");
    strcat(cmd, ssa_path);
    strcat(cmd, " -o ");
    strcat(cmd, build->options->opath);
    int e = system(cmd);
    free(cmd);
    if(e != 0) {
        eprintfln("ERROR: Failed to build with qbe");
        return false;
    }
    return true;
}
