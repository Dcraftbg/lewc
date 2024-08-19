#include "build.h"

#define state_add_return(state, id) build_add_return((state)->build, (state)->fid, (state)->head, id)
#define state_add_int_add(state, v0, v1) build_add_int_add((state)->build, (state)->fid, (state)->head, v0, v1) 
#define state_add_load_arg(state, arg) build_add_load_arg((state)->build, (state)->fid, (state)->head, arg) 

#define INVALID_SYMBOLID ((size_t)-1)
size_t build_symbol_table_lookup(BuildSymbolTable* symbols, Atom* symbol) {
    for(size_t i = 0; i < symbols->len; ++i) {
        if(symbols->items[i].name->len == symbol->len && memcmp(symbols->items[i].name->data, symbol->data, symbol->len) == 0) {
            return symbols->items[i].id;
        }
    }
    return INVALID_SYMBOLID;
}
void build_symbol_table_add(BuildSymbolTable* symbols, Atom* symbol, size_t id) {
    da_reserve(symbols, 1);
    size_t i = symbols->len++;
    symbols->items[i].name = symbol;
    symbols->items[i].id = id;
}
static size_t state_lookup_symbol(BuildState* state, Atom* symbol)  {
    BuildFunc* func = &state->build->funcs.items[state->fid];
    return build_symbol_table_lookup(&func->local_table, symbol);
}
size_t build_add_func(Build* build, Atom* name, typeid_t type) {
    da_reserve(&build->funcs, 1);
    size_t id = build->funcs.len++;
    memset(&build->funcs.items[id], 0, sizeof(build->funcs.items[id]));
    build->funcs.items[id].name = name;
    build->funcs.items[id].typeid = type;
    return id;
}
#define build_get_block(build, fid, head) &build->funcs.items[fid].blocks.items[head]
size_t build_add_block(Build* build, size_t fid) {
    BuildFunc* func = &build->funcs.items[fid];
    da_reserve(&func->blocks, 1);
    size_t id = func->blocks.len++;
    memset(&func->blocks.items[id], 0, sizeof(func->blocks.items[0]));
    return id;
}
size_t build_add_int_add(Build* build, size_t fid, size_t head, size_t v0, size_t v1) {
    Block *block = build_get_block(build, fid, head);
    da_reserve(block, 1);
    size_t id = block->len++;
    block->items[id].kind = BUILD_ADD_INT;
    block->items[id].v0 = v0;
    block->items[id].v1 = v1;
    return id;
}

size_t build_add_return(Build* build, size_t fid, size_t head, size_t arg) {
    Block *block = build_get_block(build, fid, head);
    da_reserve(block, 1);
    size_t id = block->len++;
    block->items[id].kind = BUILD_RETURN;
    block->items[id].arg = arg;
    return id;
}

size_t build_add_load_arg(Build* build, size_t fid, size_t head, size_t arg) {
    Block *block = build_get_block(build, fid, head);
    da_reserve(block, 1);
    size_t id = block->len++;
    block->items[id].kind = BUILD_LOAD_ARG;
    block->items[id].arg = arg;
    return id;
}

size_t build_astvalue(BuildState* state, ASTValue value);
size_t build_ast(BuildState* state, AST* ast) {
    switch(ast->kind) {
    case '+':
        size_t v0 = build_astvalue(state, ast->left);
        size_t v1 = build_astvalue(state, ast->right);
        return state_add_int_add(state, v0, v1);
    default:
        eprintfln("Unsupported ast->kind in build_ast: %d\n",ast->kind);
        if(ast->kind < 256) {
           eprintfln("Char representation: %c", (char)ast->kind);
        }
        exit(1);
    }
}
size_t build_astvalue(BuildState* state, ASTValue value) {
    static_assert(AST_VALUE_COUNT == 2, "Update build_astvalue");
    switch(value.kind) {
    case AST_VALUE_SYMBOL: {
        return state_lookup_symbol(state, value.symbol);
    } break;
    case AST_VALUE_EXPR: {
        return build_ast(state, value.ast);
    } break;
    default:
        eprintfln("%s:%u: UNREACHABLE", __FILE__, __LINE__);
        exit(1);
    }
}
void build_build(Build* build, Parser* parser) {
    BuildState state = {0};
    state.build = build;
    type_table_move(&build->type_table, &parser->type_table);

    for(size_t i = 0; i < parser->funcs.len; ++i) {
        struct FuncPair* fpair = &parser->funcs.items[i];
        Scope* scope = fpair->scope;
        Atom* name = fpair->name;
        typeid_t typeid = fpair->type;
        (void)scope;
        Type* type = type_table_get(&build->type_table, typeid);
        assert(type->core == CORE_FUNC);
        state.fid = build_add_func(build, name, typeid);
        state.head = build_add_block(build, state.fid);

        for(size_t j=0; j < type->signature.input.len; ++j) {
            if(type->signature.input.items[j].name) 
                build_symbol_table_add(&state.build->funcs.items[state.fid].local_table, type->signature.input.items[j].name, state_add_load_arg(&state, j));
        }
        for(size_t j=0; j < scope->insts.len; ++j) {
            Instruction* inst = &scope->insts.items[j];
            static_assert(INST_COUNT == 1, "Update build_build");
            switch(inst->kind) {
            case INST_RETURN:
                state_add_return(&state, build_astvalue(&state, inst->astvalue));
                break;
            default:
                eprintfln("UNHANDLED INSTRUCTION %d",inst->kind);
                exit(1);
            }
        } 
    } 
}
