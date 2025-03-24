#include "typefix.h"
#include "typeinfer.h"
bool typefix_func(ProgramState* state, Function* func);
bool typefix_ast_nonvoid(ProgramState* state, AST* ast);
bool typefix_ast(ProgramState* state, AST* ast) {
    static_assert(AST_KIND_COUNT == 10, "Update typefix_ast");
    Type* old = ast->type;
    switch(ast->kind) {
    case AST_FUNC:
        return typefix_func(state, ast->as.func);
    case AST_CAST:
        assert(ast->as.cast.into);
        return typefix_ast(state, ast->as.cast.what);
    case AST_CALL: {
        if(!typefix_ast_nonvoid(state, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typefix_ast_nonvoid(state, ast->as.call.args.items[i])) return false;
        }
    } break;
    case AST_BINOP:
        if(!typefix_ast_nonvoid(state, ast->as.binop.lhs)) return false;
        if(ast->as.binop.op == '.') break; // <- This means we're accessing a field which is special
        if(!typefix_ast_nonvoid(state, ast->as.binop.rhs)) return false;
        break;
    case AST_SUBSCRIPT:
        if(!typefix_ast_nonvoid(state, ast->as.subscript.what)) return false;
        if(!typefix_ast_nonvoid(state, ast->as.subscript.with)) return false;
        break;
    case AST_UNARY:
        if(!typefix_ast_nonvoid(state, ast->as.unary.rhs)) return false;
        break;
    case AST_NULL:
        if(!ast->type) {
            eprintfln("ERROR Failed to infer type for `null` consider casting (i.e. `cast(null, *<type>)`)");
            return false;
        }
        break;
    case AST_C_STR:
        assert(ast->type);
        break;
    case AST_SYMBOL:
        if(!ast->type) {
            eprintfln("ERROR Failed to infer type for symbol `%s`", ast->as.symbol.name->data);
            return false;
        }
        break;
    case AST_INT:
        if(!ast->type) ast->type = &type_i32;
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    if(ast->type == NULL && ast->kind != AST_CALL) {
        eprintfln("ERROR Failed to infer type for expression <TBD, dump expression>");
        return false;
    }
    if(ast->type != old) {
        infer_up_ast(state, ast->parent, ast->type);
    }
    return true;
}
bool typefix_ast_nonvoid(ProgramState* state, AST* ast) {
    if(!typefix_ast(state, ast)) return false;
    if(!ast->type) {
        eprintfln("ERROR Expected type for expression: <TBD, dump expression> but got void");
        return false;
    }
    return true;
}

bool typefix_scope(ProgramState* state, Statements* scope);
bool typefix_statement(ProgramState* state, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update typefix_statement");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast) return true;
        return typefix_ast_nonvoid(state, statement->as.ast);
    case STATEMENT_LOCAL_DEF:
        Symbol* s = statement->as.local_def.symbol;
        if(s->ast && !typefix_ast_nonvoid(state, s->ast)) return false;
        if(!s->type) {
            eprintfln("ERROR Failed to infer type for local `%s`", statement->as.local_def.name->data);
            return false;
        }
        break;
    case STATEMENT_EVAL:
        if(!typefix_ast(state, statement->as.ast)) return false;
        break;
    case STATEMENT_LOOP:
        if(!typefix_statement(state, statement->as.loop.body)) return false;
        break;
    case STATEMENT_DEFER:
        if(!typefix_statement(state, statement->as.defer.statement)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!typefix_scope(state, statement->as.scope)) return false;
        break;
    case STATEMENT_IF:
        if(!typefix_ast_nonvoid(state, statement->as.iff.cond)) return false;
        break;
    case STATEMENT_WHILE:
        if(!typefix_ast_nonvoid(state, statement->as.whil.cond)) return false;
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool typefix_scope(ProgramState* state, Statements* scope) {
    for(size_t i = 0; i < scope->len; ++i) {
        if(!typefix_statement(state, scope->items[i])) return false;
    }
    return true;
}
bool typefix_func(ProgramState* state, Function* func) {
    if(func->type->attribs & TYPE_ATTRIB_EXTERN) return true;
    return typefix_scope(state, func->scope);
}
bool typefix(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            static_assert(SYMBOL_COUNT == 2, "Update typefix");
            switch(s->kind) {
            case SYMBOL_CONSTANT:
            case SYMBOL_VARIABLE:
                if(!s->type) {
                    eprintfln("ERROR Failed to infer type for constant `%s`", spair->key->data);
                    // NOTE: Intentionally fallthrough so that the ast will report what part needs to be inferred
                }
                if(!typefix_ast_nonvoid(state, s->ast)) return false;
                assert(s->type);
                break;
            case SYMBOL_COUNT:
            default:
                unreachable("s->kind=%d", s->kind);
            }
        }
    }
    return true;
}
