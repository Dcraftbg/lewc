#include "const_eval.h"
#include "token.h"

bool const_eval_const(ProgramState* state, SymTabNode* node, Constant* c); 
// TODO: Actual decent error reporting
AST* const_eval_ast(ProgramState* state, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 6, "Update const_eval_ast");
    switch(ast->kind) {
    case AST_CALL:
        eprintfln("ERROR: Cannot call inside constant expression");
        return NULL;
    case AST_DEREF:
        eprintfln("ERROR: Cannot dereference inside constant expression");
        return NULL;
    case AST_C_STR:
        eprintfln("ERROR: C strings in constant expressions are currently forbidden. Sorry.");
        return NULL;
    case AST_SYMBOL: {
        Symbol* sym = stl_lookup(node, ast->as.symbol);
        if(sym->kind != SYMBOL_CONSTANT) {
            // FIXME: String representation in here
            eprintfln("ERROR: Cannot have non-constant symbols in constant expressions");
            return NULL;
        }
        Constant* c = sym->as.constant;
        if(!const_eval_const(state, node, c)) return NULL;
        return c->ast;
    } break;
    case AST_INT:
        return ast;
    case AST_BINOP: {
        AST *lhs = NULL, *rhs = NULL;
        if(!(lhs = const_eval_ast(state, node, ast->as.binop.lhs))) return NULL;
        if(!(rhs = const_eval_ast(state, node, ast->as.binop.lhs))) return NULL;
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
bool const_eval_const(ProgramState* state, SymTabNode* node, Constant* c) {
    if(c->evaluated) return true;
    AST* ast = const_eval_ast(state, node, c->ast);
    if(!ast) return false;
    c->ast = ast;
    c->evaluated = true;
    return true;
}
bool const_eval(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->consts.buckets.len; ++i) {
        for(
            Pair_ConstTab* cpair = state->consts.buckets.items[i].first;
            cpair;
            cpair = cpair->next
        ) {
            Constant* c = cpair->value;
            if(c->evaluated) continue;
            if(!const_eval_const(state, node, c)) return false;
        }
    }
    return true;
}
