#include "control_flow.h"
#include "darray.h"

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
        if(!cf_analyse_scope(state, statement->as.scope)) return false;
        statement->terminal = statement->as.scope->terminal;
        break;
    case STATEMENT_LOCAL_DEF:
        if(statement->as.local_def.symbol->ast) return cf_analyse_ast(state, statement->as.local_def.symbol->ast);
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool cf_analyse_scope(ProgramState* state, Statements* scope) {
    ssize_t terminal_at = -1;
    for(size_t i=0; i < scope->len; ++i) {
        if(!cf_analyse_statement(state, scope->items[i])) return false;
        if(terminal_at < 0 && scope->items[i]->terminal) {
            terminal_at = i;
        } else if (terminal_at >= 0) eprintfln("WARN: Unreachable statement!");
    }
    // FIXME: This leaks 
    // Dead code elimitation
    if(terminal_at >= 0) {
        scope->len = terminal_at + 1;
        scope->terminal = true;
    }
    return true;
}
bool cf_analyse_func(ProgramState* state, Function* func) {
    Type* type = func->type;
    assert(type->core == CORE_FUNC);
    if(type->attribs & TYPE_ATTRIB_EXTERN) return true;
    if(!cf_analyse_scope(state, func->scope)) return false;
    if(!func->scope->terminal) {
        if(!type->signature.output) {
            da_push(func->scope, statement_return(state->arena, NULL));
            return true;
        }
        eprintfln("ERROR: Missing return statement at the end of function");
        return false;
    }
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
