#include "syn_analys.h"
#include "darray.h"

#define HASHMAP_DEFINE
#include "hashmap.h"
#define SYM_TAB_ALLOC(n) malloc(n)
#define SYM_TAB_DEALLOC(ptr, size) free(ptr)
// TODO: Hash set
MAKE_HASHMAP_EX(SymTab, sym_tab, Atom*, bool, atom_hash, atom_eq, SYM_TAB_ALLOC, SYM_TAB_DEALLOC)
#undef HASHMAP_DEFINE
typedef struct {
    SymTab *items;
    size_t len, cap;
} SymTabList;
bool stl_lookup(SymTabList* list, Atom* a) {
    for(size_t i = 0; i < list->len; ++i) {
        if (sym_tab_get(&list->items[i], a)) return true;
    }
    return false;
}
// TODO: Better error messages as AST should probably store location too
bool syn_analyse_ast(SymTabList* list, AST* ast) {
    static_assert(AST_KIND_COUNT == 261, "Update syn_analyse_ast");
    switch(ast->kind) {
    case AST_CALL:
        if(!syn_analyse_ast(list, ast->as.call.what)) return false;
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            if(!syn_analyse_ast(list, ast->as.call.args.items[i])) return false;
        }
        break;
    case AST_SET:
        // For = --------------------
        // Or pointer when dereferencing
        if(ast->as.binop.lhs->kind != AST_SYMBOL) {
            eprintfln("ERROR: Cannot assign to something thats not a variable");
            return false;
        }
        // ------ For any other binop
        if(!syn_analyse_ast(list, ast->as.binop.lhs)) return false;
        if(!syn_analyse_ast(list, ast->as.binop.rhs)) return false;
        break;
    case AST_SYMBOL:
        if(!stl_lookup(list, ast->as.symbol)) {
            eprintfln("Unknown variable or function `%s`", ast->as.symbol->data);
            return false;
        }
        break;
    case AST_INT:
    case AST_C_STR:
        break;
    default:
        eprintfln("UNHANDLED AST %d", ast->kind);
        exit(1);
    }
    return true;
}
bool syn_analyse(ProgramState* state) {
    SymTabList list={0};
    da_push(&list, (SymTab){0});
    for(size_t i = 0; i < state->funcs.map.buckets.len; ++i) {
        Pair_FuncMap* fpair = state->funcs.map.buckets.items[i].first;
        while(fpair) {
            Atom* name = fpair->key;
            sym_tab_insert(&list.items[list.len-1], name, 0);
            Function* func = &fpair->value;
            Type* type = type_table_get(&state->type_table, func->type);
            assert(type->core == CORE_FUNC);
            if(!(type->attribs & TYPE_ATTRIB_EXTERN)) {
                da_push(&list, (SymTab){0});
                for(size_t j=0; j < type->signature.input.len; ++j) {
                    if(type->signature.input.items[j].name) {
                        sym_tab_insert(&list.items[list.len-1], type->signature.input.items[j].name, 0);
                    }
                }
                for(size_t j=0; j < func->scope->statements.len; ++j) {
                    Statement* statement = &func->scope->statements.items[j];
                    static_assert(STATEMENT_COUNT == 2, "Update syn_analyse");
                    switch(statement->kind) {
                    case STATEMENT_RETURN:
                        if(!syn_analyse_ast(&list, statement->ast)) return false;
                        break;
                    case STATEMENT_EVAL:
                        if(!syn_analyse_ast(&list, statement->ast)) return false;
                        break;
                    default:
                        eprintfln("UNHANDLED STATEMENT %d",statement->kind);
                        exit(1);
                    }
                } 
                sym_tab_destruct(&list.items[list.len--]);
            }
            fpair = fpair->next; 
        }
    }
    return true;
}
