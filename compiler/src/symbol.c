#include "symbol.h"

Symbol* symtab_symbol_find(SymbolTable* table, Atom* name) {
    for(size_t i = 0; i < table->len; ++i) {
        Atom* symname = table->items[i].name;
        if(symname->len == name->len && strncmp(symname->data, name->data, name->len) == 0) {
            return table->items[i].symbol;
        }
    }
    return NULL;
}
void symtab_insert(SymbolTable* table, Atom* name, Symbol* symbol) {
    da_reserve(table, 1);
    size_t id = table->len++;
    table->items[id].name = name;
    table->items[id].symbol = symbol;
}
