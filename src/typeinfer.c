#include "typeinfer.h"
#include "token.h"
#include "darray.h"
// TODO: Lots and lots of recursion in here.
// We can simplify this by collecting the ASTS we need to go down first instead of this
void infer_down_ast(Arena* arena, AST* ast, Type* type);
void typeinfer_func(Arena* arena, Function* f);
void infer_up_ast(Arena* arena, AST* ast, Type* type) {
    while(ast) {
        if(ast->type) break;
        infer_down_ast(arena, ast, type);
        ast->type = type;
        ast = ast->parent;
    }
}
void infer_symbol(Arena* arena, Symbol* s, Type* type) {
    assert(s->type == NULL);
    s->type = type;
    for(size_t i = 0; i < s->infer_asts.len; ++i) {
        s->infer_asts.items[i]->type = type;
        infer_up_ast(arena, s->infer_asts.items[i]->parent, type);
    }
    infer_down_ast(arena, s->ast, type);
    free(s->infer_asts.items);
    memset(&s->infer_asts, 0, sizeof(s->infer_asts));
}

bool try_infer_ast(Arena* arena, AST* ast);
void infer_down_ast(Arena* arena, AST* ast, Type* type) {
    if(ast->kind != AST_SYMBOL && try_infer_ast(arena, ast)) return;
    if(ast->type) return;
    static_assert(AST_KIND_COUNT == 10, "Update infer_down_ast");
    switch(ast->kind) {
    case AST_SYMBOL: {
        if(ast->as.symbol.sym->type) {
            // if(!type_eq(ast->as.symbol.sym->type, type)) {
            //     eprintf("ERROR Mismatch between symbol type "); type_dump(stderr, ast->as.symbol.sym->type); eprintf(" and type expected by expression: "); type_dump(stderr, type); eprintf(NEWLINE);
            //     return;
            // }
            ast->type = ast->as.symbol.sym->type;
            return;
        }
        infer_symbol(arena, ast->as.symbol.sym, type);
    } break;
    case AST_BINOP: {
        infer_down_ast(arena, ast->as.binop.lhs, type);
        infer_down_ast(arena, ast->as.binop.rhs, type);
        ast->type = type;
    } break;
    case AST_UNARY: {
        ast->type = type;
        if(ast->as.unary.op == '*') {
            infer_down_ast(arena, ast->as.unary.rhs, ast->type->core == CORE_PTR 
                                                        ? type_ptr(arena, ast->type->inner_type, ast->type->ptr_count + 1)
                                                        : type_ptr(arena, ast->type, 1));
            return;
        }
        if(ast->as.unary.op == '&') {
            if(ast->type->core != CORE_PTR) {
                eprintf("ERROR not pointer");
                return;
            }
            infer_down_ast(arena, ast->as.unary.rhs, ast->type->ptr_count == 1 ? ast->type : type_ptr(arena, ast->type->inner_type, ast->type->ptr_count - 1));
            return;
        }
    } break;
    case AST_INT: {
        if(!type_isint(type) && type->core != CORE_PTR) {
            eprintf("ERROR Non integer type expected for integer: "); type_dump(stderr, type); eprintf(NEWLINE);
            return;
        }
        ast->type = type;
        // TODO: isize or iptrdiff or whatever
        if(type->core == CORE_PTR) {
            ast->type = &type_i32;
        }
    } break;
    case AST_NULL: {
        if(type->core == CORE_PTR) {
            ast->type = type;
        } else {
            eprintf("ERROR Non pointer type expected for null: "); type_dump(stderr, type); eprintf(NEWLINE);
        }
    } break;
    }
}
// TODO: Distinguish between errors, successful type inference and unsuccessful type inference maybe?
// I'm not sure tho. I think this is fine
bool try_infer_ast(Arena* arena, AST* ast) {
    if(ast->type) return true;
    static_assert(AST_KIND_COUNT == 10, "Update try_infer_ast");
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
        if(try_infer_ast(arena, ast->as.call.what)) {
            Type* signature_type = ast->as.call.what->type;
            // TODO: I don't know how to report this
            if(!signature_type || signature_type->core != CORE_FUNC) return true;
            FuncSignature* signature = &signature_type->signature;
            // if(signature->input.len != ast->as.call.args.len) return true;
            for(size_t i = 0; i < ast->as.call.args.len; ++i) {
                if(i >= signature->input.len) 
                    try_infer_ast(arena, ast->as.call.args.items[i]);
                else 
                    infer_down_ast(arena, ast->as.call.args.items[i], signature->input.items[i].type);
            }
            ast->type = signature->output;
            return true;
        }
    } break;
    // TODO: Comparison expressions should automatically set the resulting type to bool (same with || and &&)
    case AST_BINOP: {
        switch(ast->as.binop.op) {
        case TOKEN_NEQ:
        case TOKEN_EQEQ:
        case TOKEN_LTEQ:
        case TOKEN_GTEQ:
        case '<':
        case '>':
            ast->type = &type_bool;
            if(try_infer_ast(arena, ast->as.binop.lhs)) {
                if(!ast->as.binop.lhs->type) return true;
                infer_down_ast(arena, ast->as.binop.rhs, ast->as.binop.lhs->type);
            } else if(try_infer_ast(arena, ast->as.binop.rhs)) {
                if(!ast->as.binop.rhs->type) return true;
                infer_down_ast(arena, ast->as.binop.lhs, ast->as.binop.rhs->type);
            } else return false;
            return true;
        case TOKEN_BOOL_AND:
        case TOKEN_BOOL_OR:
            ast->type = &type_bool;
            infer_down_ast(arena, ast->as.binop.lhs, &type_bool);
            infer_down_ast(arena, ast->as.binop.rhs, &type_bool);
            return true;
        case '.': {
            // FIXME: You can't infer type for field if you don't know the type for the structure. Seems pretty reasonable
            // but I guess it could be kinda wrong
            if(!try_infer_ast(arena, ast->as.binop.lhs)) return true;
            if(!ast->as.binop.lhs->type) return true;
            Struct* s;
            Type* type = ast->as.binop.lhs->type;
            switch(type->core) {
            case CORE_PTR:
            case CORE_STRUCT:
                if(type->core == CORE_PTR && type->inner_type->core == CORE_STRUCT) {
                    s = &type->inner_type->struc;
                } else if (type->core == CORE_STRUCT) {
                    s = &type->struc;
                } else return true; 
                Member* m = members_get(&s->members, ast->as.binop.rhs->as.symbol.name);
                if(!m) return true;
                ast->type = m->type;
                break;
            case CORE_CONST_ARRAY:
                Atom* name = ast->as.binop.rhs->as.symbol.name;
                if(strcmp(name->data, "data") == 0) {
                    ast->type = type_ptr(arena, type->array.of, 1);
                }
                break;
            default:
                return true;
            }
            return true;  
        } 
        default:
            if(try_infer_ast(arena, ast->as.binop.lhs)) {
                if(!(ast->type = ast->as.binop.lhs->type)) return true;
                infer_down_ast(arena, ast->as.binop.rhs, ast->type);
                return true;
            } else if(try_infer_ast(arena, ast->as.binop.rhs)) {
                if(!(ast->type = ast->as.binop.rhs->type)) return true;
                infer_down_ast(arena, ast->as.binop.lhs, ast->type);
                return true;
            }
        }
    } break;
    // TODO: ! must set the resulting type to bool 
    case AST_UNARY: {
        if(try_infer_ast(arena, ast->as.unary.rhs)) {
            if(!(ast->as.unary.rhs->type)) return true;
            if(ast->as.unary.op == '*') {
                //                                            NOTE: this will throw an error at the typechecking phase
                //                                            Cuz I don't really know how else to report this error currently
                if(ast->as.unary.rhs->type->core != CORE_PTR) ast->type = NULL;
                else ast->type = ast->as.unary.rhs->type->ptr_count == 1 ? ast->as.unary.rhs->type->inner_type : type_ptr(arena, ast->as.unary.rhs->type->inner_type, ast->as.unary.rhs->type->ptr_count-1);
                return true;
            }
        }
    } break;
    case AST_SUBSCRIPT: {
        if(try_infer_ast(arena, ast->as.subscript.what)) {
            if(!(ast->as.subscript.what)) return true;
            //                                            NOTE: this will throw an error at the typechecking phase
            //                                            Cuz I don't really know how else to report this error currently
            if(ast->as.subscript.what->type->core != CORE_CONST_ARRAY) ast->type = NULL;
            else ast->type = ast->as.subscript.what->type->array.of;
            infer_down_ast(arena, ast->as.subscript.with, &type_i32);
            return true;
        }
    } break;
    case AST_CAST: {
        try_infer_ast(arena, ast->as.cast.what);
        // infer_down_ast(arena, ast->as.cast.what, ast->as.cast.into);
        ast->type = ast->as.cast.into;
        return true;
    } break;
    default:
    }
    return false;
}

void typeinfer_scope(Arena* arena, Type* return_type, Statements* scope);
void typeinfer_statement(Arena* arena, Type* return_type, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update typeinfer_statement");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(statement->as.ast) infer_down_ast(arena, statement->as.ast, return_type);
        break;
    case STATEMENT_LOCAL_DEF: {
        Symbol* s = statement->as.local_def.symbol;
        if(s->ast) {
            if(s->type) {
                infer_down_ast(arena, s->ast, s->type);
            } else {
                try_infer_ast(arena, s->ast);
                if(s->ast->type) infer_symbol(arena, s, s->ast->type);
            }
        }
    } break;
    case STATEMENT_EVAL:
        try_infer_ast(arena, statement->as.ast);
        break;
    case STATEMENT_LOOP:
        typeinfer_statement(arena, return_type, statement->as.loop.body);
        break;
    case STATEMENT_SCOPE:
        typeinfer_scope(arena, return_type, statement->as.scope);
        break;
    case STATEMENT_IF:
        try_infer_ast(arena, statement->as.iff.cond);
        typeinfer_statement(arena, return_type, statement->as.iff.body);
        if(statement->as.iff.elze) typeinfer_statement(arena, return_type, statement->as.iff.elze);
        break;
    case STATEMENT_WHILE:
        try_infer_ast(arena, statement->as.whil.cond);
        typeinfer_statement(arena, return_type, statement->as.whil.body);
        break;
    case STATEMENT_DEFER:
        typeinfer_statement(arena, return_type, statement->as.defer.statement);
        break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
}
void typeinfer_scope(Arena* arena, Type* return_type, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        typeinfer_statement(arena, return_type, scope->items[j]);
    }
}
void typeinfer_func(Arena* arena, Function* func) {
    if(func->type->attribs & TYPE_ATTRIB_EXTERN) return;
    typeinfer_scope(arena, func->type->signature.output, func->scope);
}
bool typeinfer_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s = module->symbols.items[i].symbol;
        static_assert(SYMBOL_COUNT == 2, "Update typeinfer");
        switch(s->kind) {
        case SYMBOL_CONSTANT:
        case SYMBOL_VARIABLE:
            if(s->ast->kind == AST_FUNC) {
                typeinfer_func(module->arena, s->ast->as.func);
            }
            // fallthrough
            if(s->ast) {
                if(s->type) {
                    infer_down_ast(module->arena, s->ast, s->type);
                } else {
                    try_infer_ast(module->arena, s->ast);
                    if(s->ast->type) infer_symbol(module->arena, s, s->ast->type);
                }
            }
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind = %d", s->kind);
        }
    }
    return true;
}
bool typeinfer(ProgramState* state) {
    return typeinfer_module(state->main);
}
