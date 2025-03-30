#pragma once
#include "progstate.h"
#include <stdbool.h>
bool typeinfer(ProgramState* state);
void infer_up_ast(Arena* arena, AST* ast, Type* type);
void infer_symbol(Arena* arena, Symbol* s, Type* type);
