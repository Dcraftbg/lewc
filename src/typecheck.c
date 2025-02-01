#include "typecheck.h"
#include "token.h"
// TODO: Actually decent error reporting
bool typecheck_ast(ProgramState* state, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 6, "Update typecheck_ast");
    switch(ast->kind) {
    case AST_CALL: {
        if(!typecheck_ast(state, node, ast->as.call.what)) return false;
        Type *t = ast->as.call.what->type;
        if(t->core != CORE_FUNC) {
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
        ast->type = t->signature.output;
        FuncSignature* signature = &t->signature;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typecheck_ast(state, node, ast->as.call.args.items[i])) return false;
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
        if(!typecheck_ast(state, node, ast->as.binop.lhs)) return false;
        if(!typecheck_ast(state, node, ast->as.binop.rhs)) return false;
        switch(ast->as.binop.op) {
        case '=':
        case '&':
        case '+':
            ast->type = ast->as.binop.lhs->type;
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
            ast->type = &type_bool;
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
        if(!typecheck_ast(state, node, ast->as.unary.rhs)) return false;
        AST* rhs = ast->as.unary.rhs;
        switch(ast->as.unary.op) {
        case '*':
            if(!rhs->type || rhs->type->core != CORE_PTR) {
                eprintf("Trying to dereference an expression of type "); type_dump(stderr, rhs->type); eprintf("\n");
                return false;
            }
            ast->type = rhs->type->ptr_count == 1 ? rhs->type->inner_type : type_ptr(state->arena, rhs->type->inner_type, rhs->type->ptr_count-1);
            break;
        default:
            unreachable("unary.op=%c", ast->as.unary.op);
        }
    } break;
    case AST_SYMBOL: 
        ast->type = stl_lookup(node, ast->as.symbol)->type;
        break;
    case AST_INT:
        // eprintf("AST_INT. type="); type_dump(stderr, ast->as.integer.type); eprintf("\n");
        ast->type = ast->as.integer.type ?
                            ast->as.integer.type :
                            &type_i32;
        break;
    case AST_C_STR:
        ast->type = type_ptr(state->arena, &type_u8, 1);
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    return true;
}
bool typecheck_scope(ProgramState* state, SymTabNode* node, Type* return_type, Statements* scope);
bool typecheck_statement(ProgramState* state, SymTabNode* node, Type* return_type, Statement* statement) {
    static_assert(STATEMENT_COUNT == 4, "Update syn_analyse");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast && !return_type) return true;
        if(!statement->as.ast && return_type) {
            eprintf("Expected return value as function returns "); type_dump(stderr, return_type); eprintfln(" but got empty return");
            return false;
        }
        if(!typecheck_ast(state, node, statement->as.ast)) return false;
        if(statement->as.ast && !return_type) {
            eprintf("Expected empty return as the function doesn't have a return type but got "); type_dump(stderr, statement->as.ast->type); eprintf(NEWLINE);
            return false;
        }
        if(!type_eq(statement->as.ast->type, return_type)) {
            eprintf("Return type mismatch. Expected "); type_dump(stderr, return_type); eprintf(" but got "); type_dump(stderr, statement->as.ast->type); eprintf("\n");
            return false;
        }
        break;
    case STATEMENT_EVAL:
        if(!typecheck_ast(state, node, statement->as.ast)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!typecheck_scope(state, node, return_type, statement->as.scope)) return false;
        break;
    case STATEMENT_WHILE: {
        if(!typecheck_ast(state, node, statement->as.whil.cond)) return false;
        AST* cond = statement->as.whil.cond;
        if(!type_eq(cond->type, &type_bool)) {
            eprintf("While loop condition has type "); type_dump(stderr, cond->type); eprintfln(" Expected type bool");
            return false;
        }
        if(!typecheck_statement(state, node, return_type, statement->as.whil.body)) return false;
    } break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool typecheck_scope(ProgramState* state, SymTabNode* node, Type* return_type, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        if(!typecheck_statement(state, node, return_type, scope->items[j])) return false;
    }
    return true;
}
bool typecheck(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
    for(size_t i = 0; i < state->consts.buckets.len; ++i) {
        Pair_ConstTab* cpair = state->consts.buckets.items[i].first;
        while(cpair) {
            Constant* c = cpair->value;
            if(!typecheck_ast(state, node, c->ast)) return false;
            if(!type_eq(c->type, c->ast->type)) {
                eprintfln("ERROR: Mismatch in definition of constant `%s`", cpair->key->data);
                eprintf(" Constant type: "); type_dump(stderr, c->type); eprintf(NEWLINE);
                eprintf(" Value: "); type_dump(stderr, c->ast->type); eprintf(NEWLINE);
                return false;
            }
            cpair = cpair->next; 
        }
    }
    for(size_t i = 0; i < state->funcs.buckets.len; ++i) {
        for(
            Pair_FuncMap* fpair = state->funcs.buckets.items[i].first;
            fpair;
            fpair = fpair->next
        ) {
            SymTabNode* old = node;
            Function* func = &fpair->value;
            if(func->type->attribs & TYPE_ATTRIB_EXTERN) continue;
            node = func->symtab_node;
            if(!typecheck_scope(state, node, func->type->signature.output, func->scope)) return false;
            node = old;
        }
    }
    return true;
}
