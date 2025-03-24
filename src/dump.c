#if 0
#include "dump.h"
#include "scratch.h"
#include "strutils.h"

void dump_statement(int indent, Statement* st) {
    if(st->kind != STATEMENT_SCOPE) fprintf(stderr, "%*s", indent, "");
    switch(st->kind) {
    case STATEMENT_IF:
        fprintf(stderr, "if ");
        dump_ast(st->as.iff.cond);
        fprintf(stderr, "\n");
        dump_statement(indent, st->as.iff.body);
        if(st->as.iff.elze) {
            fprintf(stderr, "\n");
            fprintf(stderr, "%*s", indent, "");
            fprintf(stderr, "else "); dump_statement(indent, st->as.iff.body);
        }
        break;
    case STATEMENT_RETURN:
        fprintf(stderr, "return");
        if(st->as.ast) {
            fprintf(stderr, " ");
            dump_ast(st->as.ast);
        }
        fprintf(stderr, ";");
        break;
    case STATEMENT_EVAL:
        dump_ast(st->as.ast);
        fprintf(stderr, ";");
        break;
    case STATEMENT_SCOPE:
        dump_scope(indent, st->as.scope);
        break;
    case STATEMENT_WHILE:
        fprintf(stderr, "while ");
        dump_ast(st->as.whil.cond);
        fprintf(stderr, "\n");
        dump_statement(indent, st->as.whil.body);
        break;
    case STATEMENT_LOCAL_DEF:
        fprintf(stderr, "%s : ", st->as.local_def.name->data); type_dump(stderr, st->as.local_def.symbol->type);
        if(st->as.local_def.symbol->ast) {
            fprintf(stderr, " = ");
            dump_ast(st->as.local_def.symbol->ast);
        }
        break;
    case STATEMENT_DEFER:
        fprintf(stderr, "/*defer*/");
        break;
    }
    
}
void dump_scope(int indent, Statements* scope) {
    fprintf(stderr, "%*s{\n", indent, "");
    for(size_t i = 0; i < scope->len; ++i) {
        dump_statement(indent + 4, scope->items[i]);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "%*s}", indent, "");
}
void dump_ast(AST* ast) {
    switch(ast->kind) {
    case AST_FUNC:
        type_dump(stderr, ast->type);
        fprintf(stderr, " ");
        if(ast->type && ast->type->attribs & TYPE_ATTRIB_EXTERN) {
            fprintf(stderr, ";");
            return;
        }
        dump_scope(0, ast->as.func->scope);
        break;
    case AST_INT:
        if(ast->as.integer.value > 100) fprintf(stderr, "0x%08lX", ast->as.integer.value);
        else fprintf(stderr, "%ld", ast->as.integer.value);
        type_dump(stderr, ast->type);
        break;
    case AST_C_STR: {
        ScratchBuf sb = { 0 };
        strescape(ast->as.str.str, ast->as.str.len, &sb);
        fprintf(stderr, "c\"%s\"", sb.data);
    } break;
    case AST_SYMBOL:
        fprintf(stderr, "%s", ast->as.symbol.name->data);
        break;
    case AST_UNARY:
        fprintf(stderr, "(");
        fprintf(stderr, "%c", ast->as.unary.op);
        dump_ast(ast->as.unary.rhs);
        fprintf(stderr, ")");
        break;
    case AST_BINOP:
        fprintf(stderr, "(");
        dump_ast(ast->as.binop.lhs);
        if(ast->as.binop.op != '.') fprintf(stderr, " ");
        fprintf(stderr, "%c", ast->as.binop.op);
        if(ast->as.binop.op != '.') fprintf(stderr, " ");
        dump_ast(ast->as.binop.rhs);
        fprintf(stderr, ")");
        break;
    case AST_CALL:
        dump_ast(ast->as.call.what);
        fprintf(stderr, "(");
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(i > 0) fprintf(stderr, ", ");
            dump_ast(ast->as.call.args.items[i]);
        }
        fprintf(stderr, ")");
        break;
    }
}
void dump(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            static_assert(SYMBOL_COUNT == 2, "Update defer_expand");
            switch(s->kind) {
            case SYMBOL_CONSTANT:
            case SYMBOL_VARIABLE:
                if(s->type && s->type->attribs & TYPE_ATTRIB_EXTERN) fprintf(stderr, "extern ");
                fprintf(stderr, "%s :", spair->key->data);
                if(!s->type || s->type->core != CORE_FUNC) type_dump(stderr, s->type);
                fprintf(stderr, ": ");
                dump_ast(s->ast);
                fprintf(stderr, "\n");
                break;
            case SYMBOL_COUNT:
            default:
                unreachable("s->kind=%d", s->kind);
            }
        }
    }
}
#endif
