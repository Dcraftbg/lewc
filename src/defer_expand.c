#include "defer_expand.h"
#include "darray.h"
// TODO: Use a dequeue maybe
typedef struct {
    Statements* items;
    size_t len, cap;
} DeferStack;
size_t defer_stack_push(DeferStack* stack) {
    da_reserve(stack, 1);
    size_t idx = stack->len++;
    memset(&stack->items[idx], 0, sizeof(stack->items[idx])); 
    return idx;
}
void defer_stack_pop(DeferStack* stack) {
    size_t idx = --stack->len;
    free(stack->items[idx].items);
    memset(&stack->items[idx], 0, sizeof(stack->items[idx])); 
}
void defer_stack_destroy(DeferStack* stack) {
    assert(stack->len == 0);
    free(stack->items);
}
void defer_expand_ast(Arena* arena, AST* ast);
void defer_expand_statement(Arena* arena, DeferStack* ds, Statement** statement);
void defer_expand_scope(Arena* arena, DeferStack* ds, Statements* scope);
void defer_expand_body(Arena* arena, DeferStack* ds, Statement** body) {
    size_t idx = defer_stack_push(ds);
    if((*body)->kind == STATEMENT_SCOPE) defer_expand_scope(arena, ds, (*body)->as.scope);
    else defer_expand_statement(arena, ds, body);
    Statements* deferred = &ds->items[idx];
    if((!(*body)->terminal) && deferred->len) {
        // TODO: we can probably get away with expanding singular defer into its normal statement
        // if we encounter it here. I don't think its gonna make all that big of a difference
        // its not like the scope takes up all that much memory anyway
        if((*body)->kind != STATEMENT_SCOPE) {
            Statement* scope = statement_scope(arena);
            da_push(scope->as.scope, (*body));
            *body = scope;
        }
        Statements* scope = (*body)->as.scope;
        da_reserve(scope, deferred->len);
        for(int i = ((int)deferred->len)-1; i >= 0; --i) {
            scope->items[scope->len++] = deferred->items[i];
        }
    }
    defer_stack_pop(ds);
}
void defer_expand_statement(Arena* arena, DeferStack* ds, Statement** statement) {
    assert(ds->len);
    static_assert(STATEMENT_COUNT == 8, "Update defer_expand_statement");
    switch((*statement)->kind) {
    case STATEMENT_DEFER:
        da_push(&ds->items[ds->len-1], (*statement)->as.defer.statement);
        break;
    case STATEMENT_WHILE:
        defer_expand_ast(arena, (*statement)->as.whil.cond);
        defer_expand_body(arena, ds, &(*statement)->as.whil.body);
        break;
    case STATEMENT_EVAL:
        defer_expand_ast(arena, (*statement)->as.ast);
        break;
    case STATEMENT_RETURN:
        if((*statement)->as.ast) defer_expand_ast(arena, (*statement)->as.ast);
        Statement* ret = *statement;
        Statement* scope_statement = statement_scope(arena);
        Statements* scope = (scope_statement)->as.scope;
        for(int i = ((int)ds->len) - 1; i >= 0; --i) {
            Statements* deferred = &ds->items[i];
            if(deferred->len == 0) continue;
            da_reserve(scope, deferred->len);
            for(int j = ((int)deferred->len) - 1; j >= 0; --j) {
                scope->items[scope->len++] = deferred->items[j];
            }
        }
        da_push(scope, ret);
        if(scope->len > 1) *statement = scope_statement;
        else free(scope->items);
        break;
    case STATEMENT_IF:
        defer_expand_ast(arena, (*statement)->as.iff.cond);
        defer_expand_body(arena, ds, &(*statement)->as.iff.body);
        if((*statement)->as.iff.elze) defer_expand_body(arena, ds, &(*statement)->as.iff.elze);
        break;
    case STATEMENT_LOOP:
        defer_expand_body(arena, ds, &(*statement)->as.loop.body);
        break;
    case STATEMENT_LOCAL_DEF:
        break;
    case STATEMENT_SCOPE:
        defer_expand_body(arena, ds, statement);
        break;
    default:
        unreachable("(*statement)->kind=%d", (*statement)->kind);
        break;
    }
}
void defer_expand_scope(Arena* arena, DeferStack* ds, Statements* scope) {
    for(size_t j=0; j < scope->len; ++j) {
        defer_expand_statement(arena, ds, &scope->items[j]);
    }
}
void defer_expand_func(Arena* arena, Function* func) {
    if(!func->scope) return;
    DeferStack ds = { 0 };
    defer_stack_push(&ds);
        defer_expand_scope(arena, &ds, func->scope);
    defer_stack_pop(&ds);
    defer_stack_destroy(&ds);
}
void defer_expand_ast(Arena* arena, AST* ast) {
    if(!ast) return;
    static_assert(AST_KIND_COUNT == 10, "Update defer_expand_ast");
    switch(ast->kind) {
    case AST_FUNC:
        defer_expand_func(arena, ast->as.func);
        break;
    default:
    }
}

void defer_expand_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s  = module->symbols.items[i].symbol;
        static_assert(SYMBOL_COUNT == 2, "Update defer_expand");
        switch(s->kind) {
        case SYMBOL_CONSTANT:
        case SYMBOL_VARIABLE:
            defer_expand_ast(module->arena, s->ast);
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind=%d", s->kind);
        }
        
    }
}
void defer_expand(ProgramState* state) {
    return defer_expand_module(state->main);
}
