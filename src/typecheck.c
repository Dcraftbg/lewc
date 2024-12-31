#include "typecheck.h"
// TODO: Actually decent error reporting
bool typecheck_ast(ProgramState* state, SymTabNode* node, AST* ast) {
    static_assert(AST_KIND_COUNT == 261, "Update typecheck_ast");
    switch(ast->kind) {
    case AST_CALL: {
        if(!typecheck_ast(state, node, ast->as.call.what)) return false;
        Type *t = ast->as.call.what->type;
        if(t->core != CORE_FUNC) {
            eprintfln("ERROR: Tried to call something that is not a function");
            return false;
        }
        if(ast->as.call.args.len < t->signature.input.len) {
            eprintfln("ERROR: Too few argument in function call.");
            return false;
        }
        if(ast->as.call.args.len > t->signature.input.len) {
            eprintfln("ERROR: Too many argument in function call.");
            eprintfln("Function expects %zu arguments, but got %zu", t->signature.input.len, ast->as.call.args.len);
            return false;
        }
        ast->type = t->signature.output;
        FuncSignature* signature = &t->signature;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typecheck_ast(state, node, ast->as.call.args.items[i])) return false;
            if(!type_eq(ast->as.call.args.items[i]->type, signature->input.items[i].type)) {
                eprintfln("Argument %zu did not match type!", i);
                return false;
            }
        }
    } break;
    case AST_SET:
        if(!typecheck_ast(state, node, ast->as.binop.lhs)) return false;
        if(!typecheck_ast(state, node, ast->as.binop.rhs)) return false;
        ast->type = ast->as.binop.lhs->type;
        if(!type_eq(ast->as.binop.lhs->type, ast->as.binop.rhs->type)) {
            eprintfln("Trying to assign to a different type");
            return false;
        }
        break;
    case '+':
        if(!typecheck_ast(state, node, ast->as.binop.lhs)) return false;
        if(!typecheck_ast(state, node, ast->as.binop.rhs)) return false;
        ast->type = ast->as.binop.lhs->type;
        if(!type_eq(ast->as.binop.lhs->type, ast->as.binop.rhs->type)) {
            eprintfln("Trying to add two different types together with '+'");
            return false;
        }
        if(!type_isbinary(ast->as.binop.lhs->type)) {
            eprintfln("ERROR: We don't support addition between nonbinary types");
            type_dump(stderr, ast->as.binop.lhs->type); eprintf(" + "); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
            return false;
        }
        break;
    case AST_SYMBOL: 
        ast->type = stl_lookup(node, ast->as.symbol)->type;
        break;
    case AST_INT:
        // TODO: Multiple integer types
        ast->type = &type_i32;
        break;
    case AST_C_STR:
        ast->type = type_ptr(state->arena, &type_u8, 1);
        break;
    default:
        eprintfln("(typecheck_ast) UNHANDLED AST %d", ast->kind);
        exit(1);
    }
    return true;
}
bool typecheck(ProgramState* state) {
    SymTabNode* node = &state->symtab_root;
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
            for(size_t j = 0; j < func->scope->statements.len; ++j) {
                Statement* statement = &func->scope->statements.items[j];
                static_assert(STATEMENT_COUNT == 2, "Update syn_analyse");
                switch(statement->kind) {
                case STATEMENT_RETURN:
                    if(!typecheck_ast(state, node, statement->ast)) return false;
                    break;
                case STATEMENT_EVAL:
                    if(!typecheck_ast(state, node, statement->ast)) return false;
                    break;
                default:
                    eprintfln("UNHANDLED STATEMENT %d",statement->kind);
                    exit(1);
                }
            } 
            node = old;
        }
    }
    return true;
}
