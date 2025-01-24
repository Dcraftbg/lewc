#define SYMTAB_DEFINE
#include "progstate.h"
#include "syn_analys.h"
#include "darray.h"

typedef struct {
    SymTab *items;
    size_t len, cap;
} SymTabList;
Symbol* stl_lookup(SymTabNode* node, Atom* a) {
    while(node) {
        Symbol** s;
        if ((s=sym_tab_get(&node->symtab, a))) return *s;
        node = node->parent;
    }
    return false;
}
static Symbol* symbol_new(Arena* arena, Type* type) {
    Symbol* symbol = arena_alloc(arena, sizeof(*symbol));
    assert(symbol && "Ran out of memory");
    memset(symbol, 0, sizeof(*symbol));
    symbol->type = type;
    return symbol;
}
SymTabNode* symtab_node_new(SymTabNode* parent, Arena* arena) {
    SymTabNode* node = arena_alloc(arena, sizeof(*node));
    assert(node && "Ran out of memory");
    memset(node, 0, sizeof(*node));
    node->parent = parent;
    da_push(parent, node);
    return node;
}
// TODO: Better error messages as AST should probably store location too
bool syn_analyse_ast(SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 262, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_CALL:
        if(!syn_analyse_ast(node, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!syn_analyse_ast(node, ast->as.call.args.items[i])) return false;
        }
        break;
    case AST_SET:
        // TODO: Make sure this isn't a function we're trying to assign to
        // For = --------------------
        // Or pointer when dereferencing
        if(ast->as.binop.lhs->kind != AST_SYMBOL) {
            eprintfln("ERROR: Cannot assign to something thats not a variable");
            return false;
        }
    case '+':
        // ------ For any other binop
        if(!syn_analyse_ast(node, ast->as.binop.lhs)) return false;
        if(!syn_analyse_ast(node, ast->as.binop.rhs)) return false;
        break;
    case AST_DEREF:
        if(!syn_analyse_ast(node, ast->as.deref.what)) return false;
        break;
    case AST_SYMBOL:
        if(!stl_lookup(node, ast->as.symbol)) {
            eprintfln("Unknown variable or function `%s`", ast->as.symbol->data);
            return false;
        }
        break;
    case AST_INT:
    case AST_C_STR:
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    return true;
}
bool syn_analyse_scope(SymTabNode* node, Statements* scope) {
    for(size_t j=0; j < scope->len; ++j) {
        Statement* statement = scope->items[j];
        static_assert(STATEMENT_COUNT == 3, "Update syn_analyse");
        switch(statement->kind) {
        case STATEMENT_RETURN:
            if(!syn_analyse_ast(node, statement->as.ast)) return false;
            break;
        case STATEMENT_EVAL:
            if(!syn_analyse_ast(node, statement->as.ast)) return false;
            break;
        case STATEMENT_SCOPE:
            if(!syn_analyse_scope(node, statement->as.scope)) return false;
            break;
        default:
            unreachable("statement->kind=%d", statement->kind);
        }
    }
    return true;
}
bool syn_analyse(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->funcs.buckets.len; ++i) {
        Pair_FuncMap* fpair = state->funcs.buckets.items[i].first;
        while(fpair) {
            sym_tab_insert(&node->symtab, fpair->key, symbol_new(state->arena, fpair->value.type));
            fpair = fpair->next; 
        }
    }
    for(size_t i = 0; i < state->funcs.buckets.len; ++i) {
        Pair_FuncMap* fpair = state->funcs.buckets.items[i].first;
        while(fpair) {
            Function* func = &fpair->value;
            Type* type = func->type;
            assert(type->core == CORE_FUNC);
            if(!(type->attribs & TYPE_ATTRIB_EXTERN)) {
                func->symtab_node = node = symtab_node_new(node, state->arena);
                for(size_t j=0; j < type->signature.input.len; ++j) {
                    if(type->signature.input.items[j].name) {
                        sym_tab_insert(&node->symtab, type->signature.input.items[j].name, symbol_new(state->arena, type->signature.input.items[j].type));
                    }
                }
                if(!syn_analyse_scope(node, func->scope)) return false;
                node = node->parent;
            }
            fpair = fpair->next; 
        }
    }
    return true;
}
