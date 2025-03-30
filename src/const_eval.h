#pragma once
#include "progstate.h"
#include <stdbool.h>
bool const_eval_module(Module* module);
bool const_eval(ProgramState* state);
