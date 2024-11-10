#include "build.h"

#define state_add_return(state, id) build_add_return((state)->build, (state)->fid, (state)->head, id)
#define state_add_int_add(state, v0, v1) build_add_int_add((state)->build, (state)->fid, (state)->head, v0, v1) 
#define state_add_store_int(state, v0, v1) build_add_store_int((state)->build, (state)->fid, (state)->head, v0, v1) 
#define state_add_load_int(state, v0) build_add_load_int((state)->build, (state)->fid, (state)->head, v0) 

#define state_add_load_arg(state, arg) build_add_load_arg((state)->build, (state)->fid, (state)->head, arg) 
#define state_add_alloca(state, typeid) build_add_alloca((state)->build, (state)->fid, (state)->head, typeid)

#define state_add_global_arr(state, typeid, data, len) build_add_global_arr((state)->build, typeid, data, len)
#define state_add_get_addr_of(state, globalid) build_add_get_addr_of((state)->build, (state)->fid, (state)->head, globalid)

#define state_add_directcall(state, what, args) build_add_directcall((state)->build, (state)->fid, (state)->head, what, args)
#define state_add_const_int(state, type, value) build_add_const_int((state)->build, (state)->fid, (state)->head, type, value)

#define INVALID_SYMBOLID ((size_t)-1)
// #define INVALID_SYMBOL (BuildSymbol){.id=INVALID_SYMBOLID, .allocation=0}
BuildSymbol* build_symbol_table_lookup(BuildSymbolTable* symbols, Atom* symbol) {
    for(size_t i = 0; i < symbols->len; ++i) {
        if(symbols->items[i].name->len == symbol->len && memcmp(symbols->items[i].name->data, symbol->data, symbol->len) == 0) {
            return &symbols->items[i].symbol;
        }
    }
    return NULL;
}
void build_symbol_table_add(BuildSymbolTable* symbols, Atom* symbol, BuildSymbol bs) {
    da_reserve(symbols, 1);
    size_t i = symbols->len++;
    symbols->items[i].name = symbol;
    symbols->items[i].symbol = bs;
}
static BuildSymbol* state_lookup_symbol(BuildState* state, Atom* symbol)  {
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
static size_t build_get_funcid_by_name(Build* build, Atom* name) {
    for(size_t i = 0; i < build->funcs.len; ++i) {
        if(strcmp(build->funcs.items[i].name->data, name->data) == 0) return i;
    }
    return INVALID_SYMBOLID;
}
#define build_get_func(build, fid) &build->funcs.items[fid]
size_t build_add_block(Build* build, size_t fid) {
    BuildFunc* func = &build->funcs.items[fid];
    da_reserve(&func->blocks, 1);
    size_t id = func->blocks.len++;
    memset(&func->blocks.items[id], 0, sizeof(func->blocks.items[0]));
    return id;
}
static size_t build_add_global(Build* build, BuildGlobal global) {
    da_reserve(&build->globals, 1);
    size_t id = build->globals.len++;
    build->globals.items[id] = global;
    return id;
}
#define build_get_global(build, id) &build->globals.items[id]
size_t build_add_global_arr(Build* build, typeid_t typeid, void* data, size_t len) {
    return build_add_global(build, (BuildGlobal){.kind=GLOBAL_ARRAY, .array={.typeid=typeid, .data=data, .len=len}});
}
size_t build_add_inst(Build* build, size_t fid, size_t head, BuildInst inst) {
    BuildFunc* func = build_get_func(build, fid);
    Block *block = &func->blocks.items[head];//
    da_reserve(block, 1);
    size_t id = func->ip++;
    block->items[block->len++] = inst;
    return id;
}
size_t build_add_int_add(Build* build, size_t fid, size_t head, size_t v0, size_t v1) {
    BuildInst inst = {0};
    inst.kind = BUILD_ADD_INT;
    inst.v0 = v0;
    inst.v1 = v1;
    return build_add_inst(build, fid, head, inst);
}

size_t build_add_store_int(Build* build, size_t fid, size_t head, size_t v0, size_t v1) {
    BuildInst inst = {0};
    inst.kind = BUILD_STORE_INT;
    inst.v0 = v0;
    inst.v1 = v1;
    return build_add_inst(build, fid, head, inst);
}
size_t build_add_load_int(Build* build, size_t fid, size_t head, size_t arg) {
    BuildInst inst = {0};
    inst.kind = BUILD_LOAD_INT;
    inst.arg = arg;
    return build_add_inst(build, fid, head, inst);
}



size_t build_add_return(Build* build, size_t fid, size_t head, size_t arg) {
    BuildInst inst = {0};
    inst.kind = BUILD_RETURN;
    inst.arg = arg;
    return build_add_inst(build, fid, head, inst);
}

size_t build_add_load_arg(Build* build, size_t fid, size_t head, size_t arg) {
    BuildInst inst = {0};
    inst.kind = BUILD_LOAD_ARG;
    inst.arg = arg;
    return build_add_inst(build, fid, head, inst);
}
size_t build_add_alloca(Build* build, size_t fid, size_t head, typeid_t typeid) {
    BuildInst inst = {0};
    inst.kind = BUILD_ALLOCA;
    inst.type = typeid;
    return build_add_inst(build, fid, head, inst);
}

size_t build_add_get_addr_of(Build* build, size_t fid, size_t head, size_t globalid) {
    BuildInst inst = {0};
    inst.kind = BUILD_GET_ADDR_OF;
    inst.globalid = globalid;
    return build_add_inst(build, fid, head, inst);
}

size_t build_add_directcall(Build* build, size_t fid, size_t head, size_t what, BuildCallArgs args) {
    BuildInst inst = {0};
    inst.kind = BUILD_CALL_DIRECTLY;
    inst.directcall.fid = what;
    inst.directcall.args = args;
    return build_add_inst(build, fid, head, inst);
}
size_t build_add_const_int(Build* build, size_t fid, size_t head, typeid_t type, uint64_t value) {
    BuildInst inst = {0};
    inst.kind = BUILD_CONST_INT;
    inst.integer.type  = type;
    inst.integer.value = value;
    return build_add_inst(build, fid, head, inst);
}
// build_add_directcall((state)->build, (state)->fid, (state)->head, fid, args)

size_t build_ast(BuildState* state, AST* ast) {
    switch(ast->kind) {
    case '+':
        size_t v0 = build_ast(state, ast->as.binop.lhs);
        size_t v1 = build_ast(state, ast->as.binop.rhs);
        return state_add_int_add(state, v0, v1);
    case AST_CALL:
        assert(ast->as.call.what->kind == AST_SYMBOL);
        Atom* symbol = ast->as.call.what->as.symbol;
        size_t whatid = build_get_funcid_by_name(state->build, symbol);
        if(whatid == INVALID_SYMBOLID) {
            eprintfln("Invalid function name %s", symbol->data);
            for(size_t i = 0; i < state->build->funcs.len; ++i) {
                eprintfln("%zu> Function %s", i, state->build->funcs.items[i].name->data);
            }
            exit(1);
        }
        BuildCallArgs args={0};
        for(size_t i = 0; i < ast->as.call.args.len; ++i) {
            da_push(&args, build_ast(state, ast->as.call.args.items[i]));
        }
        return state_add_directcall(state, whatid, args);
    case AST_SYMBOL: {
        BuildSymbol* sym = state_lookup_symbol(state, ast->as.symbol);
        static_assert(BUILD_SYM_ALLOC_COUNT == 3);
        switch(sym->allocation) {
        case BUILD_SYM_ALLOC_DIRECT:
            return sym->id;
        case BUILD_SYM_ALLOC_PTR:
            return state_add_load_int(state, sym->id);
        default:
            eprintfln("Unhandled allocation mechanism %d", sym->allocation);
            abort();
        }
    } break;
    case AST_C_STR: {
        size_t globalid = state_add_global_arr(state, BUILTIN_U8, (void*)ast->as.str.str, ast->as.str.len+1);
        return state_add_get_addr_of(state, globalid);
    } break;
    case AST_INT: {
        return state_add_const_int(state, BUILTIN_I32, ast->as.integer.value);
    } break;
    default:
        eprintfln("Unsupported ast->kind in build_aststate->: %d\n",ast->kind);
        if(ast->kind < 256) {
           eprintfln("Char representation: %c", (char)ast->kind);
        }
        exit(1);
    }
}
void build_build(Build* build, ProgramState* pstate) {
    BuildState state = {0};
    state.build = build;
    type_table_move(&build->type_table, &pstate->type_table);
    for(size_t i = 0; i < pstate->funcs.map.buckets.len; ++i) {
        Pair_FuncMap* fpair = pstate->funcs.map.buckets.items[i].first;
        while(fpair) {
            build_add_func(build, fpair->key, fpair->value.type);
            fpair = fpair->next;
        }
    }
    for(size_t i = 0; i < pstate->funcs.map.buckets.len; ++i) {
        Pair_FuncMap* fpair = pstate->funcs.map.buckets.items[i].first;
        while(fpair) {
            Atom* name = fpair->key;
            Scope* scope = fpair->value.scope;
            typeid_t typeid = fpair->value.type;
            (void)scope;
            Type* type = type_table_get(&build->type_table, typeid);
            assert(type->core == CORE_FUNC);
            state.fid = build_get_funcid_by_name(build, name);
            if(!(type->attribs & TYPE_ATTRIB_EXTERN)) {
                state.head = build_add_block(build, state.fid);
                for(size_t j=0; j < type->signature.input.len; ++j) {
                    if(type->signature.input.items[j].name) {
                        Type* argt = type_table_get(&build->type_table, type->signature.input.items[j].typeid);
                        size_t alloca = state_add_alloca(&state, type->signature.input.items[j].typeid);

                        switch(argt->core) {
                        case CORE_I32:
                            size_t larg = state_add_load_arg(&state, j);
                            state_add_store_int(&state, alloca, larg);
                            BuildSymbol sym = {
                                .allocation = BUILD_SYM_ALLOC_PTR,
                                .id = alloca,
                            };
                            build_symbol_table_add(&state.build->funcs.items[state.fid].local_table, type->signature.input.items[j].name, sym);
                            break;
                        default:
                            eprintfln("Unhandled type in signature loading in build_build: %d for function %s", argt->core, name->data);
                            exit(1);
                        }
                    }
                }
                for(size_t j=0; j < scope->statements.len; ++j) {
                    Statement* statement = &scope->statements.items[j];
                    static_assert(STATEMENT_COUNT == 2, "Update build_build");
                    switch(statement->kind) {
                    case STATEMENT_RETURN:
                        state_add_return(&state, build_ast(&state, statement->ast));
                        break;
                    case STATEMENT_EVAL:
                        build_ast(&state, statement->ast);
                        break;
                    default:
                        eprintfln("UNHANDLED STATEMENT %d",statement->kind);
                        exit(1);
                    }
                } 
            }
            fpair = fpair->next; 
        }
    }
}
