// FIXME: FIX THE SYMTAB THINGY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include "constants.h"
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
static Symbol* symbol_new_func(Arena* arena, Type* type) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_FUNCTION;
    return me;
}
static Symbol* symbol_new_var(Arena* arena, Type* type) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_VARIABLE;
    return me;
}
static Symbol* symbol_new_constant(Arena* arena, Constant* constant) {
    Symbol* me = symbol_new(arena, constant->type);
    if(!me) return NULL;
    me->kind = SYMBOL_CONSTANT;
    me->as.constant = constant;
    return me;
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
    static_assert(AST_KIND_COUNT == 6, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_CALL:
        if(!syn_analyse_ast(node, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!syn_analyse_ast(node, ast->as.call.args.items[i])) return false;
        }
        break;
    // TODO: For = force the lhs to be either SYMBOL (non constant/function!!) or *
    case AST_BINOP:
        // ------ For any other binop
        if(!syn_analyse_ast(node, ast->as.binop.lhs)) return false;
        if(!syn_analyse_ast(node, ast->as.binop.rhs)) return false;
        if(ast->as.binop.op == '=') {
            AST* lhs = ast->as.binop.lhs;
            switch(lhs->kind) {
            case AST_SYMBOL: {
                if(lhs->as.symbol.sym->kind != SYMBOL_VARIABLE) {
                    eprintfln("Cannot assign to non-variable `%s`", lhs->as.symbol.name->data);
                    return false;
                }
            } break;
            case AST_UNARY: {
                if(lhs->as.unary.op != '*') {
                    eprintfln("Can only assign to variables or dereferences. found unary: %c", lhs->as.unary.op);
                    return false;
                }
            } break;
            default:
                // TODO: Better error messages
                eprintfln("Can only assign to variables or dereferences. found: %d", lhs->kind);
                return false;
            }
        }
        break;
    case AST_UNARY:
        if(!syn_analyse_ast(node, ast->as.unary.rhs)) return false;
        break;
    case AST_SYMBOL:
        if(!(ast->as.symbol.sym = stl_lookup(node, ast->as.symbol.name))) {
            eprintfln("Unknown variable or function `%s`", ast->as.symbol.name->data);
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

bool syn_analyse_scope(ProgramState* state, SymTabNode* node, Statements* scope);
bool syn_analyse_statement(ProgramState* state, SymTabNode* node, Statement* statement) {
    static_assert(STATEMENT_COUNT == 6, "Update syn_analyse");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast) return true;
        if(!syn_analyse_ast(node, statement->as.ast)) return false;
        break;
    case STATEMENT_EVAL:
        if(!syn_analyse_ast(node, statement->as.ast)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!syn_analyse_scope(state, node, statement->as.scope)) return false;
        break;
    case STATEMENT_LOOP:
        if(!syn_analyse_statement(state, node, statement->as.loop.body)) return false;
        break;
    case STATEMENT_WHILE:
        if(!syn_analyse_ast(node, statement->as.whil.cond)) return false;
        if(!syn_analyse_statement(state, node, statement->as.whil.body)) return false;
        break;
    case STATEMENT_LOCAL_DEF:
        sym_tab_insert(&node->symtab, statement->as.local_def.name, symbol_new_var(state->arena, statement->as.local_def.type));
        if(statement->as.local_def.init && !syn_analyse_ast(node, statement->as.local_def.init)) return false;
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool syn_analyse_scope(ProgramState* state, SymTabNode* node, Statements* scope) {
    for(size_t j=0; j < scope->len; ++j) {
        if(!syn_analyse_statement(state, node, scope->items[j])) return false;
    }
    return true;
}
bool syn_analyse(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->consts.buckets.len; ++i) {
        Pair_ConstTab* cpair = state->consts.buckets.items[i].first;
        while(cpair) {
            sym_tab_insert(&node->symtab, cpair->key, symbol_new_constant(state->arena, cpair->value));
            cpair = cpair->next; 
        }
    }
    for(size_t i = 0; i < state->funcs.buckets.len; ++i) {
        Pair_FuncMap* fpair = state->funcs.buckets.items[i].first;
        while(fpair) {
            sym_tab_insert(&node->symtab, fpair->key, symbol_new_func(state->arena, fpair->value.type));
            fpair = fpair->next; 
        }
    }
    for(size_t i = 0; i < state->consts.buckets.len; ++i) {
        Pair_ConstTab* cpair = state->consts.buckets.items[i].first;
        while(cpair) {
            if(!syn_analyse_ast(node, cpair->value->ast)) return false;
            cpair = cpair->next; 
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
                        sym_tab_insert(&node->symtab, type->signature.input.items[j].name, symbol_new_var(state->arena, type->signature.input.items[j].type));
                    }
                }
                if(!syn_analyse_scope(state, node, func->scope)) return false;
                if(!type->signature.output && (func->scope->len < 0 || func->scope->items[func->scope->len-1]->kind != STATEMENT_RETURN)) {
                    da_push(func->scope, statement_return(state->arena, NULL));
                }
                node = node->parent;
            }
            fpair = fpair->next; 
        }
    }
    return true;
}
