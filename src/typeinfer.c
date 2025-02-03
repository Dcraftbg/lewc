#include "typeinfer.h"
#include "token.h"
#include "darray.h"
// TODO: Lots and lots of recursion in here.
// We can simplify this by collecting the ASTS we need to go down first instead of this
void infer_down_ast(AST* ast, Type* type);
void infer_up_ast(AST* ast, Type* type) {
    while(ast) {
        if(ast->type) break;
        infer_down_ast(ast, type);
        ast = ast->parent;
    }
}
void infer_down_ast(AST* ast, Type* type) {
    if(ast->type) return;
    ast->type = type;
    switch(ast->kind) {
    case AST_SYMBOL: {
        Symbol* s = ast->as.symbol.sym;
        if(s->type) {
            ast->type = s->type;
            return;
        }
        s->type = type;
        for(size_t i = 0; i < s->infer_asts.len; ++i) {
            s->infer_asts.items[i]->type = type;
            infer_up_ast(s->infer_asts.items[i]->parent, type);
        }
        free(s->infer_asts.items);
        memset(&s->infer_asts, 0, sizeof(s->infer_asts));
    } break;
    case AST_BINOP: {
        infer_down_ast(ast->as.binop.lhs, type);
        infer_down_ast(ast->as.binop.rhs, type);
    } break;
    case AST_UNARY: {
        infer_down_ast(ast->as.unary.rhs, type);
    } break;
    }
}
// TODO: Distinguish between errors, successful type inference and unsuccessful type inference maybe?
// I'm not sure tho. I think this is fine
bool try_infer_ast(ProgramState* state, AST* ast) {
    if(ast->type) return true;
    switch(ast->kind) {
    case AST_SYMBOL: {
        Symbol* s = ast->as.symbol.sym;
        if(s->type) {
            ast->type = s->type;
            return true;
        }
        da_push(&s->infer_asts, ast);
    } break;
    case AST_CALL: {
        if(try_infer_ast(state, ast->as.call.what)) {
            Type* signature_type = ast->as.call.what->type;
            // TODO: I don't know how to report this
            if(!signature_type || signature_type->core != CORE_FUNC) return true;
            FuncSignature* signature = &signature_type->signature;
            if(signature->input.len != ast->as.call.args.len) return true;
            for(size_t i = 0; i < signature->input.len; ++i) {
                infer_down_ast(ast->as.call.args.items[i], signature->input.items[i].type);
            }
            ast->type = signature->output;
            return true;
        }
    } break;
    case AST_BINOP: {
        if(try_infer_ast(state, ast->as.binop.lhs)) {
            if(!(ast->type = ast->as.binop.lhs->type)) return true;
            infer_down_ast(ast->as.binop.rhs, ast->type);
            return true;
        } else if(try_infer_ast(state, ast->as.binop.rhs)) {
            if(!(ast->type = ast->as.binop.rhs->type)) return true;
            infer_down_ast(ast->as.binop.lhs, ast->type);
            return true;
        }
    } break;
    case AST_UNARY: {
        if(try_infer_ast(state, ast->as.unary.rhs)) {
            if(!(ast->as.unary.rhs->type)) return true;
            if(ast->as.unary.op == '*') {
                //                                            NOTE: this will throw an error at the typechecking phase
                //                                            Cuz I don't really know how else to report this error currently
                if(ast->as.unary.rhs->type->core != CORE_PTR) ast->type = NULL;
                else ast->type = ast->as.unary.rhs->type->ptr_count == 1 ? ast->as.unary.rhs->type->inner_type : type_ptr(state->arena, ast->as.unary.rhs->type->inner_type, ast->as.unary.rhs->type->ptr_count-1);
                return true;
            }
        }
    } break;
    default:
    }
    return false;
}
bool typeinfer(ProgramState* state) {
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            if(s->kind != SYMBOL_CONSTANT || s->kind != SYMBOL_FUNCTION) continue;
            if(s->as.init.ast) try_infer_ast(state, s->as.init.ast);
        }
    }
    for(size_t i = 0; i < state->symtab_root.symtab.buckets.len; ++i) {
        for(
            Pair_SymTab* spair = state->symtab_root.symtab.buckets.items[i].first;
            spair;
            spair = spair->next
        ) {
            Symbol* s = spair->value;
            if(s->kind != SYMBOL_CONSTANT || s->kind != SYMBOL_FUNCTION) continue;
            if(!s->type) {
                eprintfln("ERROR: Failed to infer type for %s. Please specify explicitly", spair->key->data);
                return false;
            }
        }
    }
    return true;
}
