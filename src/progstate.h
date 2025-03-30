#pragma once
#include "module.h"
#include "statement.h"
#include "type.h"
#include "ast.h"
#include "func.h"
#include "syn_analys.h"
#include "constants.h"
typedef struct ProgramState ProgramState;
// TODO: For optimising iteration of functions, constants etc.
// Maybe we can just keep track of them in a list (i.e. array of Symbol*)
// That way the main way to lookup a symbol is through the symbol table
// but for actually iterating we can just go through a list hopefully
struct ProgramState {
    Module* main;
    Arena* arena;
};
