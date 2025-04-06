#include "typecheck.h"
#include "token.h"

bool typecheck_func(Arena* arena, Function* func);
bool typecheck_get_member_of(const Location* loc, Type* type, Atom* member) {
    if(!type) {
        eprintfln("ERROR %s: Trying to get member `%s` of void expression!", tloc(loc), member->data);
        return false;
    }
    switch(type->core) {
    case CORE_PTR:
        if(type->ptr_count > 1) {
            eprintf("ERROR %s: Trying to get member `%s` of ", tloc(loc), member->data); type_dump(stderr, type); eprintf(NEWLINE);
            switch(type->inner_type->core) {
            case CORE_STRUCT:
            case CORE_CONST_ARRAY:
                eprintfln("NOTE: Maybe you meant to dereference it?");
                break;
            }
            return false;
        }
        switch(type->inner_type->core) {
        case CORE_STRUCT:
        case CORE_CONST_ARRAY:
            return typecheck_get_member_of(loc, type->inner_type, member);
        }
        eprintf  ("ERROR %s: Trying to get member `%s` of ", tloc(loc), member->data); type_dump(stderr, type); eprintf(NEWLINE);
        eprintfln("You can only access members to structures, constant arrays (data, len) or pointers to any of the previous.");
        return false;
    case CORE_STRUCT: {
        Struct *s = &type->struc;
        Member* m = members_get(&s->members, member);
        if(!m) {
            eprintf("ERROR %s: Unknown member `%s` of ", tloc(loc), member->data); type_dump(stderr, type); eprintf(NEWLINE);
            return false;
        }
    } break;
    case CORE_CONST_ARRAY: {
        if(strcmp(member->data, "data") != 0 && strcmp(member->data, "len") != 0)  {
            eprintfln("ERROR %s: Unknown member `%s` of constant array", tloc(loc), member->data);
            return false;
        }
    } break;
    default:
        eprintf  ("ERROR %s: Trying to get member `%s` of ", tloc(loc), member->data); type_dump(stderr, type); eprintf(NEWLINE);
        eprintfln("You can only access members to structures, constant arrays (data, len) or pointers to any of the previous.");
        return false;
    }
    return true;
}
// TODO: Actually decent error reporting
bool typecheck_ast(Arena* arena, AST* ast) {
    if(!ast) return false;
    static_assert(AST_KIND_COUNT == 11, "Update typecheck_ast");
    switch(ast->kind) {
    case AST_FUNC:
        return typecheck_func(arena, ast->as.func);
    case AST_STRUCT_LITERAL: {
        if(!ast->type || ast->type->core != CORE_STRUCT) {
            eprintf("ERROR %s: Trying to create structure literal of non structure type: ", tloc(&ast->loc)); type_dump(stderr, ast->type); eprintf(NEWLINE);
            return false;
        }
        assert(ast->type && ast->type->core == CORE_STRUCT);
        StructLiteral* lit = &ast->as.struc_literal;
        Struct* struc = &ast->type->struc;
        bool valid = true;
        for(size_t i = 0; i < lit->fields.len; ++i) {
            if(!typecheck_ast(arena, lit->fields.items[i].value)) return false;
        }
        // TODO: Here you'd fill up the fields with the 
        // default value and typecheck that
        if(lit->fields.len < struc->fields.len) {
            eprintfln("ERROR %s: Too few fields in structure literal.", tloc(&ast->loc));
            valid = false;
        }
        if(lit->fields.len > struc->fields.len) {
            eprintfln("ERROR %s: Too many fields in structure literal.", tloc(&ast->loc));
            valid = false;
        }
        // TODO: Check for duplicates
        // TODO: Check for missing fields maybe?
        for(size_t i = 0; i < lit->fields.len; ++i) {
            Atom* name = lit->fields.items[i].name;
            AST* value = lit->fields.items[i].value;
            Member* m;
            if(!(m = members_get(&struc->members, name))) {
                // TODO: Better location pointing to field
                eprintfln("ERROR %s: Unknown field in structure literal `%s`", tloc(&ast->loc), name->data);
                eprintf("Structure "); type_dump(stderr, ast->type); eprintfln(" has no such field");
                valid = false;
                continue;
            }
            if(!type_eq(m->type, value->type)) {
                // TODO: Better location pointing to field
                eprintfln("ERROR %s: Type mismatch in field `%s` of struct literal", tloc(&ast->loc), name->data);
                eprintf("Field Type: "); type_dump(stderr, m->type); eprintf(NEWLINE);
                eprintf("Value Type: "); type_dump(stderr, value->type); eprintf(NEWLINE);
                valid = false;
                continue;
            }
        }
        return valid;
    } break;
    case AST_CAST: {
        if(!typecheck_ast(arena, ast->as.cast.what)) return false;
        if((!type_isbinary(ast->as.cast.what->type)) || (!type_isbinary(ast->as.cast.into))) {
            eprintfln("ERROR %s: Cannot cast between non-binary types:", tloc(&ast->loc));
            eprintf("Trying to cast "); type_dump(stderr, ast->as.cast.what->type); eprintf(" into "); type_dump(stderr, ast->as.cast.into); eprintf(NEWLINE);
            return false;
        }
    } break;
    case AST_CALL: {
        if(!typecheck_ast(arena, ast->as.call.what)) return false;
        Type *t = ast->as.call.what->type;
        if(!t || t->core != CORE_FUNC) {
            eprintf("ERROR %s: Tried to call something that is not a function (", tloc(&ast->loc)); type_dump(stderr, t); eprintfln(")");
            return false;
        }
        if(ast->as.call.args.len < t->signature.input.len) {
            eprintfln("ERROR %s: Too few argument in function call.", tloc(&ast->loc));
            goto arg_size_mismatch;
        }
        if(ast->as.call.args.len > t->signature.input.len && t->signature.variadic == VARIADIC_NONE) {
            eprintfln("ERROR %s: Too many argument in function call.", tloc(&ast->loc));
            goto arg_size_mismatch;
        }
        FuncSignature* signature = &t->signature;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!typecheck_ast(arena, ast->as.call.args.items[i])) return false;
            if(i >= signature->input.len) continue;
            if(!type_eq(ast->as.call.args.items[i]->type, signature->input.items[i].type)) {
                eprintfln("ERROR %s: Argument %zu did not match type!", tloc(&ast->loc), i);
                eprintf("Expected "); type_dump(stderr, signature->input.items[i].type); eprintf("\n");
                eprintf("But got  "); type_dump(stderr, ast->as.call.args.items[i]->type); eprintf("\n");
                return false;
            }
        }
        break;
    arg_size_mismatch:
        eprintf("ERROR %s: Function ", tloc(&ast->loc));type_dump(stderr, t);eprintfln(" expects %zu arguments, but got %zu", t->signature.input.len, ast->as.call.args.len);
        return false;
    }
    case AST_BINOP:
        if(!typecheck_ast(arena, ast->as.binop.lhs)) return false;
        if(!typecheck_ast(arena, ast->as.binop.rhs)) return false;
        switch(ast->as.binop.op) {
        case '=':
            if(!type_eq(ast->as.binop.lhs->type, ast->as.binop.rhs->type)) {
                eprintfln("ERROR %s: Trying to assign to a different type", tloc(&ast->loc));
                type_dump(stderr, ast->as.binop.lhs->type); eprintf(" = "); type_dump(stderr, ast->as.binop.rhs->type); eprintf(NEWLINE);
                return false;
            }
            if(ast->as.binop.lhs->kind == AST_BINOP && ast->as.binop.lhs->as.binop.op == '.' && ast->as.binop.lhs->as.binop.lhs->type->core == CORE_CONST_ARRAY) {
                eprintfln("ERROR %s: Cannot assign to `%s` of a constant array!", tloc(&ast->loc), ast->as.binop.lhs->as.binop.rhs->as.symbol.name->data);
                return false;
            }
            break;
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
                    eprintfln("ERROR %s: Trying to add two different types together with '%c'", tloc(&ast->loc), ast->as.binop.op);
                    type_dump(stderr, ast->as.binop.lhs->type); eprintf(" %c ", ast->as.binop.op); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                    return false;
                }
            }
            if(!type_isbinary(ast->as.binop.lhs->type)) {
                eprintfln("ERROR %s: We don't support addition between nonbinary types:", tloc(&ast->loc));
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
                eprintfln("ERROR %s: We don't support addition between nonbinary types:", tloc(&ast->loc));
                type_dump(stderr, ast->as.binop.lhs->type); eprintf(" == "); type_dump(stderr, ast->as.binop.rhs->type); eprintf("\n");
                return false;
            }
            break;
        case TOKEN_BOOL_AND:
        case TOKEN_BOOL_OR: {
            const char* op_str = TOKEN_BOOL_OR ? "||" : "&&";
            if(!type_eq(ast->as.binop.lhs->type, &type_bool)) {
                eprintf("You can only use booleans with %s. Left hand side was: ", op_str); type_dump(stderr, ast->as.binop.lhs->type); eprintf(NEWLINE);
                return false;
            }
            if(!type_eq(ast->as.binop.rhs->type, &type_bool)) {
                eprintf("You can only use booleans with %s. Right hand side was: ", op_str); type_dump(stderr, ast->as.binop.rhs->type); eprintf(NEWLINE);
                return false;
            }
        } break;
        case '.': { 
            assert(ast->as.binop.rhs->kind == AST_SYMBOL);
            Type* type = ast->as.binop.lhs->type;
            if(type->core != CORE_STRUCT && type->core != CORE_CONST_ARRAY) {
                if(type->core == CORE_PTR && (type->inner_type->core == CORE_STRUCT || type->inner_type->core == CORE_CONST_ARRAY)) return true;
                eprintf("ERROR %s: Trying to get member of non structure (", tloc(&ast->loc)); type_dump(stderr, type); eprintfln(") isn't permitted");
                return false;
            }
        } break;
        default:
            unreachable("ast->as.binop.op=%d", ast->as.binop.op);
        }
        break;
    case AST_SUBSCRIPT:
        if(!typecheck_ast(arena, ast->as.subscript.what)) return false;
        if(!typecheck_ast(arena, ast->as.subscript.with)) return false;
        if(!ast->as.subscript.what->type || ast->as.subscript.what->type->core != CORE_CONST_ARRAY) {
            eprintf("Trying to subscript an expression of type "); type_dump(stderr, ast->as.subscript.what->type); eprintfln(" You can only subscript arrays!");
            return false;
        }
        if(!type_isint(ast->as.subscript.with->type)) {
            eprintf("Expected the index's type to be integer but got "); type_dump(stderr, ast->as.subscript.with->type); eprintf(NEWLINE);
            return false;
        }
        // TODO: If integer type != usize. Cast to usize maybe?
        break;
    case AST_UNARY: {
        if(!typecheck_ast(arena, ast->as.unary.rhs)) return false;
        AST* rhs = ast->as.unary.rhs;
        switch(ast->as.unary.op) {
        case '*':
            if(!rhs->type || rhs->type->core != CORE_PTR) {
                eprintf("Trying to dereference an expression of type "); type_dump(stderr, rhs->type); eprintf("\n");
                return false;
            }
            break;
        case '&':
            if(!ast->type || ast->type->core != CORE_PTR) {
                eprintfln("Trying to get address, and its non pointer?!?");
                type_dump(stderr, ast->type);
                eprintf(NEWLINE);
                return false;
            }
            switch(ast->as.unary.rhs->kind) {
            case AST_SYMBOL: {
                Symbol* s    = ast->as.unary.rhs->as.symbol.sym;
                Atom* name = ast->as.unary.rhs->as.symbol.name;
                switch(s->kind) {
                case SYMBOL_CONSTANT:
                    if(s->type && s->type->core == CORE_FUNC) {
                        eprintfln("NOTE: For getting function pointers, just use their name. (&%s -> %s)", name->data, name->data);
                        return false;
                    }
                    eprintf("Cannot get address of constant `%s`", name->data); type_dump(stderr, s->type); eprintfln(NEWLINE);
                    return false;
                default:
                    break;
                }
            } break;
            case AST_BINOP: {
                AST* expr = ast->as.unary.rhs;                
                if(expr->as.binop.op != '.') unreachable("Taking address of binop != '.'");
            } break;
            default:
                unreachable("rhs.kind=%c", ast->as.unary.rhs->kind);
            }
            break;
        default:
            unreachable("unary.op=%c", ast->as.unary.op);
        }
    } break;
    case AST_NULL:
    case AST_SYMBOL: 
    case AST_INT:
    case AST_C_STR:
        break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    return true;
}
bool typecheck_scope(Arena* arena, Type* return_type, Statements* scope);
bool typecheck_statement(Arena* arena, Type* return_type, Statement* statement) {
    static_assert(STATEMENT_COUNT == 8, "Update typecheck_statement");
    switch(statement->kind) {
    case STATEMENT_RETURN:
        if(!statement->as.ast && !return_type) return true;
        if(!statement->as.ast && return_type) {
            eprintf("Expected return value as function returns "); type_dump(stderr, return_type); eprintfln(" but got empty return");
            return false;
        }
        if(!typecheck_ast(arena, statement->as.ast)) return false;
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
        if(s->ast) {
            if(!typecheck_ast(arena, s->ast)) return false;
            if(!type_eq(s->type, s->ast->type)) {
                eprintfln("Type mismatch in variable definition %s.", statement->as.local_def.name->data);
                eprintf("Variable defined as "); type_dump(stderr, s->type); eprintf(" but got "); type_dump(stderr, s->ast->type); eprintf(NEWLINE);
                return false;
            }
        }
    } break;
    case STATEMENT_EVAL:
        if(!typecheck_ast(arena, statement->as.ast)) return false;
        break;
    case STATEMENT_LOOP:
        if(!typecheck_statement(arena, return_type, statement->as.loop.body)) return false;
        break;
    case STATEMENT_DEFER: 
        if(!typecheck_statement(arena, return_type, statement->as.defer.statement)) return false;
        break;
    case STATEMENT_SCOPE:
        if(!typecheck_scope(arena, return_type, statement->as.scope)) return false;
        break;
    case STATEMENT_IF: {
        if(!typecheck_ast(arena, statement->as.iff.cond)) return false;
        AST* cond = statement->as.iff.cond;
        if(!type_eq(cond->type, &type_bool)) {
            eprintf("If loop condition has type "); type_dump(stderr, cond->type); eprintfln(" Expected type bool");
            return false;
        }
        if(!typecheck_statement(arena, return_type, statement->as.iff.body)) return false;
        if(statement->as.iff.elze && !typecheck_statement(arena, return_type, statement->as.iff.elze)) return false;
    } break;
    case STATEMENT_WHILE: {
        if(!typecheck_ast(arena, statement->as.whil.cond)) return false;
        AST* cond = statement->as.whil.cond;
        if(!type_eq(cond->type, &type_bool)) {
            eprintf("While loop condition has type "); type_dump(stderr, cond->type); eprintfln(" Expected type bool");
            return false;
        }
        if(!typecheck_statement(arena, return_type, statement->as.whil.body)) return false;
    } break;
    default:
        unreachable("statement->kind=%d", statement->kind);
    }
    return true;
}
bool typecheck_scope(Arena* arena, Type* return_type, Statements* scope) {
    for(size_t j = 0; j < scope->len; ++j) {
        if(!typecheck_statement(arena, return_type, scope->items[j])) return false;
    }
    return true;
}
bool typecheck_func(Arena* arena, Function* func) {
    if(func->type->attribs & TYPE_ATTRIB_EXTERN) return true; 
    return typecheck_scope(arena, func->type->signature.output, func->scope);
}
bool typecheck_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s  = module->symbols.items[i].symbol;
        Atom* name = module->symbols.items[i].name;
        static_assert(SYMBOL_COUNT == 3, "Update typecheck");
        switch(s->kind) {
        case SYMBOL_CONSTANT:
        case SYMBOL_VARIABLE:
        case SYMBOL_GLOBAL:
            if(!typecheck_ast(module->arena, s->ast)) return false;
            if(!type_eq(s->type, s->ast->type)) {
                eprintfln("ERROR %s: Mismatch in definition of `%s`", tloc(&s->loc), name->data);
                eprintf(" Defined type: "); type_dump(stderr, s->type); eprintf(NEWLINE);
                eprintf(" Value: "); type_dump(stderr, s->ast->type); eprintf(NEWLINE);
                return false;
            }
            break;
        case SYMBOL_COUNT:
        default:
            unreachable("s->kind=%d", s->kind);
        }
    }
    return true;
}
bool typecheck(ProgramState* state) {
    return typecheck_module(state->main);
}
