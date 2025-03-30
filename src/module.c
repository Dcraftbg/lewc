#include "module.h"
#include "syn_analys.h"
#include "control_flow.h"
#include "typeinfer.h"
#include "typefix.h"
#include "typecheck.h"
#include "const_eval.h"
#include "defer_expand.h"
Module* module_new(Arena* arena, const char* path) {
    Module* me = arena_alloc(arena, sizeof(Module));
    assert(me && "Ran out of memory");
    memset(me, 0, sizeof(*me));
    me->path = path;
    me->arena = arena;
    return me;
}

bool module_do_intermediate_steps(Module* module) {
    if(!syn_analyse_module(module))          return false;
    if(!control_flow_analyse_module(module)) return false;
    if(!typeinfer_module(module))            return false;
    if(!typefix_module(module))              return false;
    if(!typecheck_module(module))            return false;
    if(!const_eval_module(module))           return false;
    defer_expand_module(module);
    return true;
}

bool modules_join(Module* parent, Module* child) {
    for(size_t i = 0; i < child->symbols.len; ++i) {
        Symbol* s  = child->symbols.items[i].symbol;
        Atom* name = child->symbols.items[i].name;
        sym_tab_insert(&parent->symtab_root.symtab, name, s);
    }
    for(size_t i = 0; i < child->typedefs.len; ++i) {
        Type* type = child->typedefs.items[i].type;
        Atom* name = child->typedefs.items[i].name;
        type_table_insert(&parent->type_table, name->data, type);
    }
    // TODO: join types
    return true;
}
