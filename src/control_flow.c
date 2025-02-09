#include "control_flow.h"

bool cf_analyse_ast(ProgramState* state, AST* ast);
bool cf_analyse_scope(ProgramState* state, Statements* scope);
bool cf_analyse_statement(ProgramState* state, Statement* statement) {
    (void)state;
    static_assert(STATEMENT_COUNT == 6, "Update cf_analyse_statement");
    switch(statement->kind) {
    case STATEMENT_EVAL:
        if(!cf_analyse_ast(state, statement->as.ast)) return false;
        break;
    case STATEMENT_RETURN:
        if(!cf_analyse_ast(state, statement->as.ast)) return false;
        // fallthrough
    // TODO: Not true for things like continue/break which can change the control flow
    case STATEMENT_LOOP:
        statement->terminal = true;
        break;
    case STATEMENT_WHILE:
        if(!cf_analyse_ast(state, statement->as.whil.cond)) return false;
        if(!cf_analyse_statement(state, statement->as.whil.body)) return false;
        break;
    case STATEMENT_SCOPE:
        return cf_analyse_scope(state, statement->as.scope);
    case STATEMENT_LOCAL_DEF:
        if(statement->as.local_def.symbol->ast) return cf_analyse_ast(state, statement->as.local_def.symbol->ast);
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool cf_analyse_scope(ProgramState* state, Statements* scope) {
    for(size_t j=0; j < scope->len; ++j) {
        if(!cf_analyse_statement(state, scope->items[j])) return false;
    }
    return true;
}
bool cf_analyse_func(ProgramState* state, Function* func) {
    Type* type = func->type;
    assert(type->core == CORE_FUNC);
    if(type->attribs & TYPE_ATTRIB_EXTERN) return true;
    if(!cf_analyse_scope(state, func->scope)) return false;
    return true;
}
bool cf_analyse_ast(ProgramState* state, AST* ast) {
    if(!ast) return true;
    static_assert(AST_KIND_COUNT == 7, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return cf_analyse_func(state, ast->as.func);
    }
    return true;
}
bool control_flow_analyse(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            static_assert(SYMBOL_COUNT == 2, "Update syn_analyse");
            switch(s->kind) {
            case SYMBOL_VARIABLE:
            case SYMBOL_CONSTANT:
                if(!cf_analyse_ast(state, s->ast)) return false;
                break;
            case SYMBOL_COUNT:
            default:
                unreachable("s->kind = %d", s->kind);
            }
        }
    }
    return true;
}
