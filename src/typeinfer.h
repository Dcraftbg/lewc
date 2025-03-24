#pragma once
#include "progstate.h"
#include <stdbool.h>
bool typeinfer(ProgramState* state);
void infer_up_ast(ProgramState* state, AST* ast, Type* type);
void infer_symbol(ProgramState* state, Symbol* s, Type* type);
