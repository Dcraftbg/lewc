#include "typecheck.h"
#include "token.h"

bool typecheck_func(ProgramState* state, Function* func);
// TODO: Actually decent error reporting
bool typecheck_ast(ProgramState* state, AST* ast) {
    if(!ast) return false;
    static_assert(AST_KIND_COUNT == 7, "Update typecheck_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return typecheck_func(state, ast->as.func);
    case AST_CALL: {
        if(!typecheck_ast(state, ast->as.call.what)) return false;
        Type *t = ast->as.call.what->type;
        if(!t || t->core != CORE_FUNC) {
            eprintfln("ast->as.call.what.kind = %d", ast->as.call.what->kind);
            eprintfln("ast->as.call.what = %s", ast->as.call.what->as.symbol.name->data);
            eprintf("ERROR: Tried to call something that is not a function ("); type_dump(stderr, t); eprintfln(")");
            return false;
        }
        if(ast->as.call.args.len < t->signature.input.len) {
            eprintfln("ERROR: Too few argument in function call.");
            goto arg_size_mismatch;
        }
        if(ast->as.call.args.len > t->signature.input.len) {
            eprintfln("ERROR: Too many argument in function call.");
            goto arg_size_mismatch;
        }
        FuncSignature* signature = &t->signature;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typecheck_ast(state, ast->as.call.args.items[i])) return false;
            if(!type_eq(ast->as.call.args.items[i]->type, signature->input.items[i].type)) {
                eprintfln("Argument %zu did not match type!", i);
                eprintf("Expected "); type_dump(stderr, signature->input.items[i].type); eprintf("\n");
                eprintf("But got  "); type_dump(stderr, ast->as.call.args.items[i]->type); eprintf("\n");
                return false;
            }
        }
        break;
    arg_size_mismatch:
        eprintf("Function ");type_dump(stderr, t);eprintfln(" expects %zu arguments, but got %zu", t->signature.input.len, ast->as.call.args.len);
        return false;
    }
    case AST_BINOP:
        if(!typecheck_ast(state, ast->as.binop.lhs)) return false;
        if(!typecheck_ast(state, ast->as.binop.rhs)) return false;
        switch(ast->as.binop.op) {
        case '=':
        case '&':
        case '+':
        case '-':
        case '|':
        case '^':
        case '*':
        case '/':
        // TODO: Ensure some of these are on integers and not on floats
        case '%':
        case TOKEN_SHL:
        case TOKEN_SHR:
            if(!type_eq(ast->as.binop.lhs->type, ast->as.binop.rhs->type)) {
                // Allow offseting with +
                // TODO: Maybe insert a cast to isize in here 
                if(ast->as.binop.op != '+' || ast->as.binop.lhs->type->core != CORE_PTR || (!type_isbinary(ast->as.binop.rhs->type))) {
                    eprintfln("Trying to add two different types together with '%c'", ast->as.binop.op);
                    type_dump(stderr, ast->as.binop.lhs->type); eprintf(" %c ", ast->as.binop.op); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                    return false;
                }
            }
            if(!type_isbinary(ast->as.binop.lhs->type)) {
                eprintfln("ERROR: We don't support addition between nonbinary types:");
                type_dump(stderr, ast->as.binop.lhs->type); eprintf(" %c ", ast->as.binop.op); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                return false;
            }
            break;
        // FIXME: Error messages are invalid
        case TOKEN_NEQ:
        case TOKEN_EQEQ:
        case TOKEN_LTEQ:
        case TOKEN_GTEQ:
        case '<':
        case '>':
            if(!type_eq(ast->as.binop.lhs->type, ast->as.binop.rhs->type)) {
                eprintfln("Trying to add two different types together with '=='");
                type_dump(stderr, ast->as.binop.lhs->type); eprintf(" == "); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                return false;
            }
            if(!type_isbinary(ast->as.binop.lhs->type)) {
                eprintfln("ERROR: We don't support addition between nonbinary types:");
                type_dump(stderr, ast->as.binop.lhs->type); eprintf(" == "); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                return false;
            }
            break;
        default:
            unreachable("ast->as.binop.op=%d", ast->as.binop.op);
        }
        break;
    case AST_UNARY: {
        if(!typecheck_ast(state, ast->as.unary.rhs)) return false;
        AST* rhs = ast->as.unary.rhs;
        switch(ast->as.unary.op) {
        case '*':
            if(!rhs->type || rhs->type->core != CORE_PTR) {
                eprintf("Trying to dereference an expression of type "); type_dump(stderr, rhs->type); eprintf("\n");
                abort();
                return false;
            }
            break;
        default:
            unreachable("unary.op=%c", ast->as.unary.op);
        }
    } break;
    case AST_SYMBOL: 
        break;
    case AST_INT:
        break;
    case AST_C_STR:
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    return true;
}
bool typecheck_scope(ProgramState* state, Type* return_type, Statements* scope);
bool typecheck_statement(ProgramState* state, Type* return_type, Statement* statement) {
    static_assert(STATEMENT_COUNT == 6, "Update typecheck_statement");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast && !return_type) return true;
        if(!statement->as.ast && return_type) {
            eprintf("Expected return value as function returns "); type_dump(stderr, return_type); eprintfln(" but got empty return");
            return false;
        }
        if(!typecheck_ast(state, statement->as.ast)) return false;
        if(statement->as.ast && !return_type) {
            eprintf("Expected empty return as the function doesn't have a return type but got "); type_dump(stderr, statement->as.ast->type); eprintf(NEWLINE);
            return false;
        }
        if(!type_eq(statement->as.ast->type, return_type)) {
            eprintf("Return type mismatch. Expected "); type_dump(stderr, return_type); eprintf(" but got "); type_dump(stderr, statement->as.ast->type); eprintf(NEWLINE);
            return false;
        }
        break;
    case STATEMENT_LOCAL_DEF: {
        Symbol* s = statement->as.local_def.symbol;
        if(s->as.init.ast) {
            if(!typecheck_ast(state, s->as.init.ast)) return false;
            if(!type_eq(s->type, s->as.init.ast->type)) {
                eprintfln("Type mismatch in variable definition %s.", statement->as.local_def.name->data);
                eprintf("Variable defined as "); type_dump(stderr, s->type); eprintf(" but got "); type_dump(stderr, s->as.init.ast->type); eprintf(NEWLINE);
                return false;
            }
        }
    } break;
    case STATEMENT_EVAL:
        if(!typecheck_ast(state, statement->as.ast)) return false;
        break;
    case STATEMENT_LOOP:
        if(!typecheck_statement(state, return_type, statement->as.loop.body)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!typecheck_scope(state, return_type, statement->as.scope)) return false;
        break;
    case STATEMENT_WHILE: {
        if(!typecheck_ast(state, statement->as.whil.cond)) return false;
        AST* cond = statement->as.whil.cond;
        if(!type_eq(cond->type, &type_bool)) {
            eprintf("While loop condition has type "); type_dump(stderr, cond->type); eprintfln(" Expected type bool");
            return false;
        }
        if(!typecheck_statement(state, return_type, statement->as.whil.body)) return false;
    } break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool typecheck_scope(ProgramState* state, Type* return_type, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        if(!typecheck_statement(state, return_type, scope->items[j])) return false;
    }
    return true;
}
bool typecheck_func(ProgramState* state, Function* func) {
    if(func->type->attribs & TYPE_ATTRIB_EXTERN) return true; 
    return typecheck_scope(state, func->type->signature.output, func->scope);
}
bool typecheck(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            static_assert(SYMBOL_COUNT == 2, "Update typecheck");
            switch(s->kind) {
            case SYMBOL_CONSTANT:
            case SYMBOL_VARIABLE:
                if(!typecheck_ast(state, s->as.init.ast)) return false;
                if(!type_eq(s->type, s->as.init.ast->type)) {
                    eprintfln("ERROR: Mismatch in definition of `%s`", spair->key->data);
                    eprintf(" Defined type: "); type_dump(stderr, s->type); eprintf(NEWLINE);
                    eprintf(" Value: "); type_dump(stderr, s->as.init.ast->type); eprintf(NEWLINE);
                    return false;
                }
                break;
            case SYMBOL_COUNT:
            default:
                unreachable("s->kind=%d", s->kind);
            }
        }
    }
    return true;
}
