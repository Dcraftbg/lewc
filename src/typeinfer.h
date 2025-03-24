#pragma once
#include "progstate.h"
#include <stdbool.h>
bool typeinfer(ProgramState* state);
void infer_up_ast(ProgramState* state, AST* ast, Type* type);
