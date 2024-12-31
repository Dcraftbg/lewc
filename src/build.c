#include "build.h"
#include "backend/qbe.h"
bool build_build(Build* build, ProgramState* state) {
    switch(build->target->backend) {
    case BACKEND_QBE:
        return build_qbe(build, state);
    default:
        eprintfln("Unsupported backend %d (%s)", build->target->backend, backend_str(build->target->backend));
        return false;
    }
    return false;
}
