#pragma once
#include "progstate.h"
#include <stdbool.h>

bool typecheck_module(Module* module);
bool typecheck(ProgramState* state);
