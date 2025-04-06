#include "typefix.h"
#include "typeinfer.h"
bool typefix_func(Arena* arena, Function* func);
bool typefix_ast_nonvoid(Arena* arena, AST* ast);
bool typefix_ast(Arena* arena, AST* ast) {
    static_assert(AST_KIND_COUNT == 11, "Update typefix_ast");
    Type* old = ast->type;
    switch(ast->kind) {
    case AST_FUNC:
        return typefix_func(arena, ast->as.func);
    case AST_CAST:
        assert(ast->as.cast.into);
        return typefix_ast(arena, ast->as.cast.what);
    case AST_CALL: {
        if(!typefix_ast_nonvoid(arena, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typefix_ast_nonvoid(arena, ast->as.call.args.items[i])) return false;
        }
    } break;
    case AST_STRUCT_LITERAL: {
        StructLiteral* lit = &ast->as.struc_literal;
        for(size_t i = 0; i < lit->fields.len; ++i) {
            if(!typefix_ast_nonvoid(arena, lit->fields.items[i].value)) return false;
        }
    } break;
    case AST_BINOP:
        if(!typefix_ast_nonvoid(arena, ast->as.binop.lhs)) return false;
        if(ast->as.binop.op == '.') break; // <- This means we're accessing a field which is special
        if(!typefix_ast_nonvoid(arena, ast->as.binop.rhs)) return false;
        break;
    case AST_SUBSCRIPT:
        if(!typefix_ast_nonvoid(arena, ast->as.subscript.what)) return false;
        if(!typefix_ast_nonvoid(arena, ast->as.subscript.with)) return false;
        break;
    case AST_UNARY:
        if(!typefix_ast_nonvoid(arena, ast->as.unary.rhs)) return false;
        break;
    case AST_NULL:
        if(!ast->type) {
            eprintfln("ERROR %s: Failed to infer type for `null` consider casting (i.e. `cast(null, *<type>)`)", tloc(&ast->loc));
            return false;
        }
        break;
    case AST_C_STR:
        assert(ast->type);
        break;
    case AST_SYMBOL:
        if(!ast->type) {
            eprintfln("ERROR %s: Failed to infer type for symbol `%s`", tloc(&ast->loc), ast->as.symbol.name->data);
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
        eprintfln("ERROR %s: Failed to infer type for expression <TBD, dump expression>", tloc(&ast->loc));
        return false;
    }
    if(ast->type != old) {
        infer_up_ast(arena, ast->parent, ast->type);
    }
    return true;
}
bool typefix_ast_nonvoid(Arena* arena, AST* ast) {
    if(!typefix_ast(arena, ast)) return false;
    if(!ast->type) {
        eprintfln("ERROR %s: Expected type for expression: <TBD, dump expression> but got void", tloc(&ast->loc));
        return false;
    }
    return true;
}

bool typefix_scope(Arena* arena, Statements* scope);
bool typefix_statement(Arena* arena, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update typefix_statement");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast) return true;
        return typefix_ast_nonvoid(arena, statement->as.ast);
    case STATEMENT_LOCAL_DEF:
        Symbol* s = statement->as.local_def.symbol;
        if(s->ast && !typefix_ast_nonvoid(arena, s->ast)) return false;
        if(!s->type && s->ast && s->ast->type) infer_symbol(arena, s, s->ast->type);
        if(!s->type) {
            eprintfln("ERROR %s: Failed to infer type for local `%s`", tloc(&s->loc), statement->as.local_def.name->data);
            return false;
        }
        break;
    case STATEMENT_EVAL:
        if(!typefix_ast(arena, statement->as.ast)) return false;
        break;
    case STATEMENT_LOOP:
        if(!typefix_statement(arena, statement->as.loop.body)) return false;
        break;
    case STATEMENT_DEFER:
        if(!typefix_statement(arena, statement->as.defer.statement)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!typefix_scope(arena, statement->as.scope)) return false;
        break;
    case STATEMENT_IF:
        if(!typefix_ast_nonvoid(arena, statement->as.iff.cond)) return false;
        break;
    case STATEMENT_WHILE:
        if(!typefix_ast_nonvoid(arena, statement->as.whil.cond)) return false;
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool typefix_scope(Arena* arena, Statements* scope) {
    for(size_t i = 0; i < scope->len; ++i) {
        if(!typefix_statement(arena, scope->items[i])) return false;
    }
    return true;
}
bool typefix_func(Arena* arena, Function* func) {
    if(func->type->attribs & TYPE_ATTRIB_EXTERN) return true;
    return typefix_scope(arena, func->scope);
}
bool typefix_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s  = module->symbols.items[i].symbol;
        Atom* name = module->symbols.items[i].name;
        static_assert(SYMBOL_COUNT == 3, "Update typefix");
        switch(s->kind) {
        case SYMBOL_CONSTANT:
        case SYMBOL_VARIABLE:
        case SYMBOL_GLOBAL:
            if(!s->type) {
                eprintfln("ERROR %s: Failed to infer type for constant `%s`", tloc(&s->loc), name->data);
                // NOTE: Intentionally fallthrough so that the ast will report what part needs to be inferred
            }
            if(!typefix_ast_nonvoid(module->arena, s->ast)) return false;
            assert(s->type);
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind=%d", s->kind);
        }
    }
    return true;
}
bool typefix(ProgramState* state) {
    return typefix_module(state->main);
}
