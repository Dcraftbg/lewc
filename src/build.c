#include "build.h"
bool build_build(Build* build, ProgramState* state) {
    switch(build->target->backend) {
    default:
        eprintfln("Unsupported backend %d (%s)", build->target->backend, backend_str(build->target->backend));
        return false;
    }
    return false;
}
