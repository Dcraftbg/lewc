#pragma once
#include "progstate.h"
void dump_ast(AST* ast);
void dump_scope(int indent, Statements* scope);
void dump_statement(int indent, Statement* st);
void dump(ProgramState* state);
