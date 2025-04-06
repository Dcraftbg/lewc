// FIXME: FIX THE SYMTAB THINGY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include "constants.h"
#define SYMTAB_DEFINE
#include "progstate.h"
#include "syn_analys.h"
#include "darray.h"

static Symbol* stl_lookup(SymTabNode* node, Atom* a) {
    while(node) {
        Symbol** s;
        if ((s=sym_tab_get(&node->symtab, a))) return *s;
        node = node->parent;
    }
    return false;
}
SymTabNode* symtab_node_new(SymTabNode* parent, Arena* arena) {
    SymTabNode* node = arena_alloc(arena, sizeof(*node));
    assert(node && "Ran out of memory");
    memset(node, 0, sizeof(*node));
    node->parent = parent;
    da_push(parent, node);
    return node;
}
bool syn_analyse_func(Arena* arena, SymTabNode* node, Function* func);
// TODO: Better error messages as AST should probably store location too
bool syn_analyse_ast(Arena* arena, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 11, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return syn_analyse_func(arena, node, ast->as.func);
    case AST_CAST:
        return syn_analyse_ast(arena, node, ast->as.cast.what);
    case AST_CALL:
        if(!syn_analyse_ast(arena, node, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!syn_analyse_ast(arena, node, ast->as.call.args.items[i])) return false;
        }
        break;
    case AST_STRUCT_LITERAL: {
        StructLiteral* lit = &ast->as.struc_literal;
        for(size_t i = 0; i < lit->fields.len; ++i) {
            if(!syn_analyse_ast(arena, node, lit->fields.items[i].value)) return false;
        }
    } break;
    case AST_SUBSCRIPT:
        if(!syn_analyse_ast(arena, node, ast->as.subscript.what)) return false;
        if(!syn_analyse_ast(arena, node, ast->as.subscript.with)) return false;
        break;
    case AST_BINOP:
        // ------ For any other binop
        if(!syn_analyse_ast(arena, node, ast->as.binop.lhs)) return false;
        switch(ast->as.binop.op) {
        case '=': {
            if(!syn_analyse_ast(arena, node, ast->as.binop.rhs)) return false;
            AST* lhs = ast->as.binop.lhs;
            switch(lhs->kind) {
            case AST_SYMBOL: {
                if(lhs->as.symbol.sym->kind != SYMBOL_VARIABLE) {
                    eprintfln("ERROR %s: Cannot assign to non-variable `%s`", tloc(&lhs->loc), lhs->as.symbol.name->data);
                    return false;
                }
            } break;
            case AST_UNARY: {
                if(lhs->as.unary.op != '*') {
                    eprintfln("ERROR %s: Can only assign to variables, dereferences or fields. found unary: %c", tloc(&lhs->loc), lhs->as.unary.op);
                    return false;
                }
            } break;
            case AST_SUBSCRIPT:
                break;
            case AST_BINOP: {
                if(lhs->as.binop.op != '.') {
                    eprintfln("ERROR %s: Can only assign to variables, dereferences or fields. found binop: %c", tloc(&lhs->loc), lhs->as.binop.op);
                    return false;
                }
            } break;
            default:
                // TODO: Better error messages
                eprintfln("ERROR %s: Can only assign to variables, dereferences, fields or subscripts. found: %d", tloc(&lhs->loc), lhs->kind);
                return false;
            }
        } break;
        case '.': {
            AST* rhs = ast->as.binop.rhs;
            if(rhs->kind != AST_SYMBOL) {
                // TODO: Better error messages
                eprintfln("ERROR %s: Field access can only be done through symbols!", tloc(&rhs->loc));
                return false;
            }
        } break;
        default:
            if(!syn_analyse_ast(arena, node, ast->as.binop.rhs)) return false;
        }
        
        break;
    case AST_UNARY:
        if(!syn_analyse_ast(arena, node, ast->as.unary.rhs)) return false;
        if(ast->as.unary.op == '&') {
            switch(ast->as.unary.rhs->kind) {
            case AST_SYMBOL:
                break;
            case AST_BINOP:
                if(ast->as.unary.rhs->as.binop.op == '.') {
                    // eprintfln("TODO: Take address of field");
                    // return false;
                    break;
                }
                // fallthrough
            default:
                eprintfln("ERROR %s: `&` can only be used on variables or fields!", tloc(&ast->as.unary.rhs->loc));
                return false;
            }
        }
        break;
    case AST_SYMBOL:
        if(!(ast->as.symbol.sym = stl_lookup(node, ast->as.symbol.name))) {
            eprintfln("ERROR %s: Unknown variable or function `%s`", tloc(&ast->loc), ast->as.symbol.name->data);
            return false;
        }
        break;
    case AST_INT:
    case AST_C_STR:
    case AST_NULL:
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    return true;
}

bool syn_analyse_scope(Arena* arena, SymTabNode* node, Statements* scope);
bool syn_analyse_statement(Arena* arena, SymTabNode* node, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update syn_analyse");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast) return true;
        if(!syn_analyse_ast(arena, node, statement->as.ast)) return false;
        break;
    case STATEMENT_EVAL:
        if(!syn_analyse_ast(arena, node, statement->as.ast)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!syn_analyse_scope(arena, node, statement->as.scope)) return false;
        break;
    case STATEMENT_LOOP:
        if(!syn_analyse_statement(arena, node, statement->as.loop.body)) return false;
        break;
    case STATEMENT_IF:
        if(!syn_analyse_ast(arena, node, statement->as.iff.cond)) return false;
        if(!syn_analyse_statement(arena, node, statement->as.iff.body)) return false;
        if(statement->as.iff.elze && !syn_analyse_statement(arena, node, statement->as.iff.elze)) return false;
        break;
    case STATEMENT_DEFER:
        if(!syn_analyse_statement(arena, node, statement->as.defer.statement)) return false;
        break;
    case STATEMENT_WHILE:
        if(!syn_analyse_ast(arena, node, statement->as.whil.cond)) return false;
        if(!syn_analyse_statement(arena, node, statement->as.whil.body)) return false;
        break;
    case STATEMENT_LOCAL_DEF:
        sym_tab_insert(&node->symtab, statement->as.local_def.name, statement->as.local_def.symbol);
        if(statement->as.local_def.symbol->ast && !syn_analyse_ast(arena, node, statement->as.local_def.symbol->ast)) return false;
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool syn_analyse_scope(Arena* arena, SymTabNode* node, Statements* scope) {
    for(size_t j=0; j < scope->len; ++j) {
        if(!syn_analyse_statement(arena, node, scope->items[j])) return false;
    }
    return true;
}
// TODO: syntactically analyse the SymTabNode instead of body maybe? 
// Maybe we shouldn't even be creating the node here but at the level of parsing
// But I'm not too sure since at least local variables have to be collected here (even tho you can
// technically just collect everything else and leave the symbol collection for local vars
// when you get to them
bool syn_analyse_func(Arena* arena, SymTabNode* parent, Function* func) {
    Type* type = func->type;
    assert(type->core == CORE_FUNC);
    if(type->attribs & TYPE_ATTRIB_EXTERN) return true;
    func->symtab_node = symtab_node_new(parent, arena);
    for(size_t j=0; j < type->signature.input.len; ++j) {
        if(type->signature.input.items[j].name) {
            sym_tab_insert(&func->symtab_node->symtab, type->signature.input.items[j].name, symbol_new_var(arena, &type->signature.input.items[j].loc, type->signature.input.items[j].type, NULL));
        }
    }
    if(!syn_analyse_scope(arena, func->symtab_node, func->scope)) return false;
    return true;
}
bool syn_analyse_module(Module* module) {
    SymTabNode* node = &module->symtab_root;
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s  = module->symbols.items[i].symbol;
        static_assert(SYMBOL_COUNT == 3, "Update syn_analyse");
        switch(s->kind) {
        case SYMBOL_VARIABLE:
        case SYMBOL_CONSTANT:
        case SYMBOL_GLOBAL:
            if(!syn_analyse_ast(module->arena, node, s->ast)) return false;
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind = %d", s->kind);
        }
        
    }
    return true;
}
bool syn_analyse(ProgramState* state) {
    return syn_analyse_module(state->main);
}
