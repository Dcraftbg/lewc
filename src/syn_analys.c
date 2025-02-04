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
static Symbol* stl_lookup(SymTabNode* node, Atom* a) {
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
Symbol* symbol_new_func(Arena* arena, Type* type, AST* func) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_FUNCTION;
    me->as.init.ast = func;
    return me;
}
Symbol* symbol_new_var(Arena* arena, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_VARIABLE;
    me->as.init.ast = init;
    return me;
}
Symbol* symbol_new_constant(Arena* arena, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_CONSTANT;
    me->as.init.ast = init;
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

bool syn_analyse_func(ProgramState* state, Function* func);
// TODO: Better error messages as AST should probably store location too
bool syn_analyse_ast(ProgramState* state, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 7, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return syn_analyse_func(state, ast->as.func);
    case AST_CALL:
        if(!syn_analyse_ast(state, node, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!syn_analyse_ast(state, node, ast->as.call.args.items[i])) return false;
        }
        break;
    // TODO: For = force the lhs to be either SYMBOL (non constant/function!!) or *
    case AST_BINOP:
        // ------ For any other binop
        if(!syn_analyse_ast(state, node, ast->as.binop.lhs)) return false;
        if(!syn_analyse_ast(state, node, ast->as.binop.rhs)) return false;
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
        if(!syn_analyse_ast(state, node, ast->as.unary.rhs)) return false;
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
        if(!syn_analyse_ast(state, node, statement->as.ast)) return false;
        break;
    case STATEMENT_EVAL:
        if(!syn_analyse_ast(state, node, statement->as.ast)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!syn_analyse_scope(state, node, statement->as.scope)) return false;
        break;
    case STATEMENT_LOOP:
        if(!syn_analyse_statement(state, node, statement->as.loop.body)) return false;
        break;
    case STATEMENT_WHILE:
        if(!syn_analyse_ast(state, node, statement->as.whil.cond)) return false;
        if(!syn_analyse_statement(state, node, statement->as.whil.body)) return false;
        break;
    case STATEMENT_LOCAL_DEF:
        sym_tab_insert(&node->symtab, statement->as.local_def.name, statement->as.local_def.symbol);
        if(statement->as.local_def.symbol->as.init.ast && !syn_analyse_ast(state, node, statement->as.local_def.symbol->as.init.ast)) return false;
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
// TODO: syntactically analyse the SymTabNode instead of body maybe? 
// Maybe we shouldn't even be creating the node here but at the level of parsing
// But I'm not too sure since at least local variables have to be collected here (even tho you can
// technically just collect everything else and leave the symbol collection for local vars
// when you get to them
bool syn_analyse_func(ProgramState* state, Function* func) {
    Type* type = func->type;
    assert(type->core == CORE_FUNC);
    if(type->attribs & TYPE_ATTRIB_EXTERN) return true;
    func->symtab_node = symtab_node_new(&state->symtab_root, state->arena);
    for(size_t j=0; j < type->signature.input.len; ++j) {
        if(type->signature.input.items[j].name) {
            sym_tab_insert(&func->symtab_node->symtab, type->signature.input.items[j].name, symbol_new_var(state->arena, type->signature.input.items[j].type, NULL));
        }
    }
    if(!syn_analyse_scope(state, func->symtab_node, func->scope)) return false;
    if(!type->signature.output && (func->scope->len <= 0 || func->scope->items[func->scope->len-1]->kind != STATEMENT_RETURN)) {
        da_push(func->scope, statement_return(state->arena, NULL));
    }
    return true;
}
bool syn_analyse(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            static_assert(SYMBOL_COUNT == 3, "Update syn_analyse");
            switch(s->kind) {
            case SYMBOL_FUNCTION:
            case SYMBOL_VARIABLE:
            case SYMBOL_CONSTANT:
                if(!syn_analyse_ast(state, node, s->as.init.ast)) return false;
                break;
            case SYMBOL_COUNT:
            default:
                unreachable("s->kind = %d", s->kind);
            }
        }
    }
    return true;
}
