#include "const_expand.h"
#include "token.h"
// TODO: Maybe this function shouldn't modify the AST but create a new one instead?
AST* const_expand_ast(ProgramState* state, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 6, "Update const_expand_ast");
    switch(ast->kind) {
    case AST_BINOP:
        ast->as.binop.lhs = const_expand_ast(state, node, ast->as.binop.lhs);
        ast->as.binop.rhs = const_expand_ast(state, node, ast->as.binop.rhs);
        if(!ast->as.binop.lhs || !ast->as.binop.rhs) return NULL;
        return ast;
    case AST_SYMBOL: {
        Symbol* s = ast->as.symbol.sym;
        if(s->kind == SYMBOL_CONSTANT) {
            Constant* c = s->as.constant;
            return c->ast;
        }
        return ast;
    };
    case AST_C_STR:
    case AST_INT:
        return ast;
    case AST_UNARY:
        if(!(ast->as.unary.rhs = const_expand_ast(state, node, ast->as.unary.rhs))) return NULL;
        return ast;
    case AST_CALL:
        if(!(ast->as.call.what = const_expand_ast(state, node, ast->as.call.what))) return NULL;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!(ast->as.call.args.items[i] = const_expand_ast(state, node, ast->as.call.args.items[i]))) return NULL;
        }
        return ast;
    }
    unreachable("const_expand_ast non exhaustive");
}
bool const_expand_scope(ProgramState* state, SymTabNode* node, Statements* scope);
bool const_expand_statement(ProgramState* state, SymTabNode* node, Statement* statement) {
    static_assert(STATEMENT_COUNT == 6, "Update syn_analyse");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast) return true;
    case STATEMENT_EVAL:
        return (statement->as.ast = const_expand_ast(state, node, statement->as.ast)) != NULL;
    case STATEMENT_LOOP:
        return const_expand_statement(state, node, statement->as.loop.body);
    case STATEMENT_SCOPE:
        return const_expand_scope(state, node, statement->as.scope);
    case STATEMENT_WHILE: {
        if(!(statement->as.whil.cond = const_expand_ast(state, node, statement->as.whil.cond))) return false;
        return const_expand_statement(state, node, statement->as.whil.body);
    } break;
    case STATEMENT_LOCAL_DEF: {
        if(!statement->as.local_def.init) return true;
        return (statement->as.local_def.init = const_expand_ast(state, node, statement->as.local_def.init)) != NULL;
    } break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool const_expand_scope(ProgramState* state, SymTabNode* node, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        if(!const_expand_statement(state, node, scope->items[j])) return false;
    }
    return true;
}
bool const_expand(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->funcs.buckets.len; ++i) {
        for(
            Pair_FuncMap* fpair = state->funcs.buckets.items[i].first;
            fpair;
            fpair = fpair->next
        ) {
            SymTabNode* old = node;
            Function* func = &fpair->value;
            if(func->type->attribs & TYPE_ATTRIB_EXTERN) continue;
            node = func->symtab_node;
            if(!const_expand_scope(state, node, func->scope)) return false;
            node = old;
        }
    }
    return true;
}
