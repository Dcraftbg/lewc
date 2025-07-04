#include "const_eval.h"
#include "token.h"
#include "darray.h"

bool const_eval_const(Arena* arena, Constant* c); 
// TODO: is evaluating flag for circular definition errors
// TODO: Actual decent error reporting
AST* const_eval_ast(Arena* arena, AST* ast) {
    static_assert(AST_KIND_COUNT == 11, "Update const_eval_ast");
    switch(ast->kind) {
    case AST_CALL:
        eprintfln("ERROR %s: Cannot call inside constant expression", tloc(&ast->loc));
        return NULL;
    case AST_UNARY:
        eprintfln("ERROR %s: Unary operators not supported inside constant expression", tloc(&ast->loc));
        return NULL;
    case AST_C_STR:
        eprintfln("ERROR %s: C strings in constant expressions are currently forbidden. Sorry.", tloc(&ast->loc));
        return NULL;
    case AST_NULL:
        eprintfln("ERROR %s: null in constant expressions is currently forbidden. Sorry.", tloc(&ast->loc));
        return NULL;
    case AST_SUBSCRIPT:
        eprintfln("ERROR %s: subscription in constant expressions are currently forbidden. Sorry.", tloc(&ast->loc));
        return NULL;
    case AST_CAST:
        eprintfln("ERROR %s: casts in constant expressions are currently forbidden. Sorry.", tloc(&ast->loc));
        return NULL;
    case AST_STRUCT_LITERAL: {
        StructLiteral  res = { 0 };
        StructLiteral* lit = &ast->as.struc_literal;
        da_reserve(&res.fields, lit->fields.len);
        for(size_t i = 0; i < lit->fields.len; ++i) {
            Atom* name = lit->fields.items[i].name;
            AST* value = lit->fields.items[i].value;
            if(!(value=const_eval_ast(arena, value))) {
                free(res.fields.items);
                return NULL;
            }
            StructLiteralField* f = &res.fields.items[res.fields.len++];
            f->name = name;
            f->value = value;
        }
        return ast_new_struct_literal(arena, &ast->loc, ast->type, res);
    }
    case AST_SYMBOL: {
        Symbol* sym = ast->as.symbol.sym;
        if(sym->kind != SYMBOL_CONSTANT) {
            // FIXME: String representation in here
            eprintfln("ERROR %s: Cannot have non-constant symbols in constant expressions", tloc(&ast->loc));
            return NULL;
        }
        if(sym->evaluated) return sym->ast;
        AST* ast = const_eval_ast(arena, sym->ast);
        if(!ast) return NULL;
        sym->evaluated = true;
        return sym->ast = ast;
    } break;
    case AST_FUNC:
    case AST_INT:
        return ast;
    case AST_BINOP: {
        AST *lhs = NULL, *rhs = NULL;
        if(!(lhs = const_eval_ast(arena, ast->as.binop.lhs))) return NULL;
        if(!(rhs = const_eval_ast(arena, ast->as.binop.rhs))) return NULL;

        Location loc = loc_join(&ast->as.binop.lhs->loc, &ast->as.binop.rhs->loc);
        switch(ast->as.binop.op) {
        case '+': {
            assert(lhs->kind == AST_INT);
            assert(rhs->kind == AST_INT);
            return ast_new_int(arena, &loc, lhs->type, lhs->as.integer.value + rhs->as.integer.value);
        } break;
        default:
            eprintf("ERROR %s: Cannot have binary operation `", tloc(&ast->loc));
            if(ast->as.binop.op < 256) eprintf("%c", ast->as.binop.op);
            else eprintf("Unknown %08X", ast->as.binop.op);
            eprintfln("` in constant expression");
            return NULL;
        }
    } break;
    default:
        unreachable("ast->kind=%d", ast->kind);
    }
    unreachable("const_eval_ast non exhaustive");
}
bool const_eval_module(Module* module) {
    for(size_t i = 0; i < module->symbols.len; ++i) {
        Symbol* s = module->symbols.items[i].symbol;
        if(s->kind != SYMBOL_CONSTANT && s->kind != SYMBOL_GLOBAL) continue;
        if(s->evaluated) continue;
        AST* ast = const_eval_ast(module->arena, s->ast);
        if(!ast) return false;
        s->evaluated = true;
        s->ast = ast;
    }
    return true;
}
bool const_eval(ProgramState* state) {
    return const_eval_module(state->main);
}
