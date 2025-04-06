#include "control_flow.h"
#include "darray.h"

bool cf_analyse_ast(Arena* arena, AST* ast);
bool cf_analyse_scope(Arena* arena, Statements* scope);
bool cf_analyse_statement(Arena* arena, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update cf_analyse_statement");
    switch(statement->kind) {
    case STATEMENT_EVAL:
        if(!cf_analyse_ast(arena, statement->as.ast)) return false;
        break;
    case STATEMENT_RETURN:
        if(!cf_analyse_ast(arena, statement->as.ast)) return false;
        // fallthrough
    // TODO: Not true for things like continue/break which can change the control flow
    case STATEMENT_LOOP:
        statement->terminal = true;
        break;
    case STATEMENT_WHILE:
        if(!cf_analyse_ast(arena, statement->as.whil.cond)) return false;
        if(!cf_analyse_statement(arena, statement->as.whil.body)) return false;
        break;
    case STATEMENT_IF:
        if(!cf_analyse_ast(arena, statement->as.iff.cond)) return false;
        if(!cf_analyse_statement(arena, statement->as.iff.body)) return false;
        if(statement->as.iff.elze) {
            if(!cf_analyse_statement(arena, statement->as.iff.elze)) return false;
            statement->terminal = statement->as.iff.body->terminal && statement->as.iff.elze->terminal;
        }
        break;
    case STATEMENT_SCOPE:
        if(!cf_analyse_scope(arena, statement->as.scope)) return false;
        statement->terminal = statement->as.scope->terminal;
        break;
    case STATEMENT_LOCAL_DEF:
        if(statement->as.local_def.symbol->ast) return cf_analyse_ast(arena, statement->as.local_def.symbol->ast);
        break;
    case STATEMENT_DEFER:
        if(!cf_analyse_statement(arena, statement->as.defer.statement)) return false;
        if(statement->as.defer.statement->terminal) {
            eprintfln("ERROR Terminal statements are not allowed within defer yet. Sorry");
            return false;
        }
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool cf_analyse_scope(Arena* arena, Statements* scope) {
    ssize_t terminal_at = -1;
    for(size_t i=0; i < scope->len; ++i) {
        if(!cf_analyse_statement(arena, scope->items[i])) return false;
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
bool cf_analyse_func(Arena* arena, Function* func) {
    Type* type = func->type;
    assert(type->core == CORE_FUNC);
    if(type->attribs & TYPE_ATTRIB_EXTERN) return true;
    if(!cf_analyse_scope(arena, func->scope)) return false;
    if(!func->scope->terminal) {
        if(!type->signature.output) {
            da_push(func->scope, statement_return(arena, NULL));
            return true;
        }
        eprintfln("ERROR Missing return statement at the end of function");
        return false;
    }
    return true;
}
bool cf_analyse_ast(Arena* arena, AST* ast) {
    if(!ast) return true;
    static_assert(AST_KIND_COUNT == 11, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return cf_analyse_func(arena, ast->as.func);
    }
    return true;
}
bool control_flow_analyse_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s  = module->symbols.items[i].symbol;
        static_assert(SYMBOL_COUNT == 3, "Update syn_analyse");
        switch(s->kind) {
        case SYMBOL_VARIABLE:
        case SYMBOL_CONSTANT:
        case SYMBOL_GLOBAL:
            if(!cf_analyse_ast(module->arena, s->ast)) return false;
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind = %d", s->kind);
        }
    }
    return true;
}
bool control_flow_analyse(ProgramState* state) {
    return control_flow_analyse_module(state->main);
}
