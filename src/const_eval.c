#include "const_eval.h"
#include "token.h"

bool const_eval_const(ProgramState* state, Constant* c); 
// TODO: is evaluating flag for circular definition errors
// TODO: Actual decent error reporting
AST* const_eval_ast(ProgramState* state, AST* ast) {
    static_assert(AST_KIND_COUNT == 8, "Update const_eval_ast");
    switch(ast->kind) {
    case AST_CALL:
        eprintfln("ERROR: Cannot call inside constant expression");
        return NULL;
    case AST_UNARY:
        eprintfln("ERROR: Unary operators not supported inside constant expression");
        return NULL;
    case AST_C_STR:
        eprintfln("ERROR: C strings in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_SUBSCRIPT:
        eprintfln("ERROR: subscription in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_SYMBOL: {
        Symbol* sym = ast->as.symbol.sym;
        if(sym->kind != SYMBOL_CONSTANT) {
            // FIXME: String representation in here
            eprintfln("ERROR: Cannot have non-constant symbols in constant expressions");
            return NULL;
        }
        if(sym->evaluated) return sym->ast;
        AST* ast = const_eval_ast(state, sym->ast);
        if(!ast) return NULL;
        sym->evaluated = true;
        return sym->ast = ast;
    } break;
    case AST_FUNC:
    case AST_INT:
        return ast;
    case AST_BINOP: {
        AST *lhs = NULL, *rhs = NULL;
        if(!(lhs = const_eval_ast(state, ast->as.binop.lhs))) return NULL;
        if(!(rhs = const_eval_ast(state, ast->as.binop.rhs))) return NULL;
        switch(ast->as.binop.op) {
        case '+':
            assert(lhs->kind == AST_INT);
            assert(rhs->kind == AST_INT);
            return ast_new_int(state->arena, lhs->type, lhs->as.integer.value + rhs->as.integer.value);
        default:
            eprintf("ERROR: Cannot have binary operation `");
            if(ast->as.binop.op < 256) eprintf("%c", ast->as.binop.op);
            else eprintf("Unknown %08X", ast->as.binop.op);
            eprintfln("` in constant expression");
            return NULL;
        }
    } break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    unreachable("const_eval_ast non exhaustive");
}
bool const_eval(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            if(s->kind != SYMBOL_CONSTANT) continue;
            if(s->evaluated) continue;
            AST* ast = const_eval_ast(state, s->ast);
            if(!ast) return false;
            s->evaluated = true;
            s->ast = ast;
        }
    }
    return true;
}
