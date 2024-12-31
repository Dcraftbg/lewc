#define NOB_IMPLEMENTATION
#include "../../nob.h"
#include <stdint.h>
#include <stddef.h>

bool record_tests() {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd,
        "python3",
        "./rere.py",
        "record",
        "test.list",
    );
    if(!nob_cmd_run_sync(cmd)) {
       nob_cmd_free(cmd);
       return false;
    }
    nob_cmd_free(cmd);
    return true;
}
bool run_tests() {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd,
        "python3",
        "./rere.py",
        "replay",
        "test.list",
    );
    if(!nob_cmd_run_sync(cmd)) {
       nob_cmd_free(cmd);
       return false;
    }
    nob_cmd_free(cmd);
    return true;
    
}

bool nob_mkdir_if_not_exists_silent(const char *path) {
     if(nob_file_exists(path)) return true;
     return nob_mkdir_if_not_exists(path);
}

bool make_build_dirs() {
    if(!nob_mkdir_if_not_exists_silent("./bin"      )) return false;
    if(!nob_mkdir_if_not_exists_silent("./bin/tests")) return false;
    if(!nob_mkdir_if_not_exists_silent("./int"      )) return false;
    if(!nob_mkdir_if_not_exists_silent("./int/tests")) return false;
    return true;
}

const char* shift_args(int *argc, const char ***argv) {
    if((*argc) <= 0) return NULL;
    const char* arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}
int main(int argc, const char** argv) {
    if(!make_build_dirs()) return 1;
    if(!nob_file_exists("./bin/lewc")) {
        nob_log(NOB_ERROR, "You need to build the compiler first before running the tests. Please use `./build build`");
        return 1;
    }
    const char* exe = shift_args(&argc, &argv);
    assert(exe);
    const char* subcmd = shift_args(&argc, &argv);
    if(!subcmd) {
        nob_log(NOB_ERROR, "Missing subcommand!");
        return 1;
    }
    if(strcmp(subcmd, "record")==0) {
        // if(nob_needs_rebuild1("test.list.bi", "test.list"))
            if(!record_tests()) return 1;
    } else if (strcmp(subcmd, "replay") == 0) {
        if(!run_tests()) return 1;
    } else {
        nob_log(NOB_ERROR, "Unknown subcommand: %s", subcmd);
        return 1;
    }
    return 0;
}
