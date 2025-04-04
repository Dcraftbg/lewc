#include "const_eval.h"
#include "token.h"

bool const_eval_const(Arena* arena, Constant* c); 
// TODO: is evaluating flag for circular definition errors
// TODO: Actual decent error reporting
AST* const_eval_ast(Arena* arena, AST* ast) {
    static_assert(AST_KIND_COUNT == 10, "Update const_eval_ast");
    switch(ast->kind) {
    case AST_CALL:
        eprintfln("ERROR Cannot call inside constant expression");
        return NULL;
    case AST_UNARY:
        eprintfln("ERROR Unary operators not supported inside constant expression");
        return NULL;
    case AST_C_STR:
        eprintfln("ERROR C strings in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_NULL:
        eprintfln("ERROR null in constant expressions is currently forbidden. Sorry.");
        return NULL;
    case AST_SUBSCRIPT:
        eprintfln("ERROR subscription in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_CAST:
        eprintfln("ERROR casts in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_SYMBOL: {
        Symbol* sym = ast->as.symbol.sym;
        if(sym->kind != SYMBOL_CONSTANT) {
            // FIXME: String representation in here
            eprintfln("ERROR Cannot have non-constant symbols in constant expressions");
            return NULL;
        }
        if(sym->evaluated) return sym->ast;
        AST* ast = const_eval_ast(arena, sym->ast);
        if(!ast) return NULL;
        sym->evaluated = true;
        return sym->ast = ast;
    } break;
    case AST_FUNC:
    case AST_INT:
        return ast;
    case AST_BINOP: {
        AST *lhs = NULL, *rhs = NULL;
        if(!(lhs = const_eval_ast(arena, ast->as.binop.lhs))) return NULL;
        if(!(rhs = const_eval_ast(arena, ast->as.binop.rhs))) return NULL;

        Location loc = loc_join(&ast->as.binop.lhs->loc, &ast->as.binop.rhs->loc);
        switch(ast->as.binop.op) {
        case '+': {
            assert(lhs->kind == AST_INT);
            assert(rhs->kind == AST_INT);
            return ast_new_int(arena, &loc, lhs->type, lhs->as.integer.value + rhs->as.integer.value);
        } break;
        default:
            eprintf("ERROR Cannot have binary operation `");
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
bool const_eval_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s = module->symbols.items[i].symbol;
        if(s->kind != SYMBOL_CONSTANT) continue;
        if(s->evaluated) continue;
        AST* ast = const_eval_ast(module->arena, s->ast);
        if(!ast) return false;
        s->evaluated = true;
        s->ast = ast;
    }
    return true;
}
bool const_eval(ProgramState* state) {
    return const_eval_module(state->main);
}
