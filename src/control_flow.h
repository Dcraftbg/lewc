#pragma once
#include "progstate.h"
#include <stdbool.h>

bool control_flow_analyse_module(Module* module);
bool control_flow_analyse(ProgramState* state);
