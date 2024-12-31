#pragma once
#include "compile.h"
#include "progstate.h"
typedef struct {
    Target* target;
    BuildOptions* options;
} Build;
bool build_build(Build* build, ProgramState* state);
