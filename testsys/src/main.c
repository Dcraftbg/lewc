#define NOB_IMPLEMENTATION
#include "../../nob.h"
#include <stdint.h>
#include <stddef.h>

#define TEST_FOLDER "examples"
const char* all_tests[] = {
    "func_calls.lew",
    "hello.lew",
    "sum.lew",
    "int_literals.lew",
    "syntactical.lew"
};
#define python_interp(cmd) nob_cmd_append(cmd, "python3")
#define python_rere(cmd) (python_interp(cmd), nob_cmd_append(&cmd, 
void test_replay(Nob_Cmd* cmd, const char* test) {
    python_interp(cmd);
    nob_cmd_append(cmd,
        "./rere.py",
        "replay",
        nob_temp_sprintf("%s/%s.test.list", TEST_FOLDER, test),
    );
}
void test_record(Nob_Cmd* cmd, const char* test) {
    python_interp(cmd);
    nob_cmd_append(cmd,
        "./rere.py",
        "record",
        nob_temp_sprintf("%s/%s.test.list", TEST_FOLDER, test),
    );
}
const char* shift_args(int *argc, const char ***argv) {
    if((*argc) <= 0) return NULL;
    const char* arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}
typedef struct {
    const char* path;
    bool failed;
    Nob_Proc proc;
} Test;
typedef struct {
    Test* items;
    size_t count, capacity;
} Tests;
bool record_tests(int argc, const char** argv) {
    Tests tests = {0};
    Nob_Cmd cmd = {0};
    if(argc) {
        Test test;
        while((test.path=shift_args(&argc, &argv))) {
            cmd.count = 0;
            test.failed = false;  
            test_record(&cmd, test.path);
            test.proc = nob_cmd_run_async(cmd);
            if(test.proc == NOB_INVALID_PROC) {
                nob_log(NOB_ERROR, "Failed to spawn test `%s`", test.path);
                continue;
            }
            nob_da_append(&tests, test);
        }
    } else {
        Test test;
        for(size_t i = 0; i < NOB_ARRAY_LEN(all_tests); ++i) {
            test.path = all_tests[i];
            cmd.count = 0;
            test.failed = false;  
            test_record(&cmd, test.path);
            test.proc = nob_cmd_run_async(cmd);
            if(test.proc == NOB_INVALID_PROC) {
                nob_log(NOB_ERROR, "Failed to spawn test `%s`", test.path);
                continue;
            }
            nob_da_append(&tests, test);
        }
    }
    nob_da_free(cmd);
    size_t failed_tests = 0;
    for(size_t i = 0; i < tests.count; ++i) {
        if((tests.items[i].failed = !nob_proc_wait(tests.items[i].proc))) {
            failed_tests++;
        }
    }
    for(size_t i = 0; i < tests.count; ++i) {
        if(tests.items[i].failed) {
            nob_log(NOB_ERROR, "Test %zu (%s) failed to record", i, tests.items[i].path);
        }
    }
    if(failed_tests) {
        nob_log(NOB_ERROR, "%zu/%zu tests failed", failed_tests, tests.count);
        nob_da_free(tests);
        return false;
    }
    nob_da_free(tests);
    nob_log(NOB_INFO, "OK recorded all tests (%zu/%zu)", tests.count, tests.count);
    return true;
}
bool run_tests(int argc, const char** argv) {
    Tests tests = {0};
    Nob_Cmd cmd = {0};
    if(argc) {
        Test test;
        while((test.path=shift_args(&argc, &argv))) {
            cmd.count = 0;
            test.failed = false;  
            test_replay(&cmd, test.path);
            test.proc = nob_cmd_run_async(cmd);
            if(test.proc == NOB_INVALID_PROC) {
                nob_log(NOB_ERROR, "Failed to spawn test `%s`", test.path);
                continue;
            }
            nob_da_append(&tests, test);
        }
    } else {
        Test test;
        for(size_t i = 0; i < NOB_ARRAY_LEN(all_tests); ++i) {
            test.path = all_tests[i];
            cmd.count = 0;
            test.failed = false;  
            test_replay(&cmd, test.path);
            test.proc = nob_cmd_run_async(cmd);
            if(test.proc == NOB_INVALID_PROC) {
                nob_log(NOB_ERROR, "Failed to spawn test `%s`", test.path);
                continue;
            }
            nob_da_append(&tests, test);
        }
    }
    nob_da_free(cmd);
    size_t failed_tests = 0;
    for(size_t i = 0; i < tests.count; ++i) {
        if((tests.items[i].failed = !nob_proc_wait(tests.items[i].proc))) {
            failed_tests++;
        }
    }
    for(size_t i = 0; i < tests.count; ++i) {
        if(tests.items[i].failed) {
            nob_log(NOB_ERROR, "Test %zu (%s) failed", i, tests.items[i].path);
        }
    }
    if(failed_tests) {
        nob_log(NOB_ERROR, "%zu/%zu tests failed", failed_tests, tests.count);
        nob_da_free(tests);
        return false;
    }
    nob_da_free(tests);
    nob_log(NOB_INFO, "OK replayed all tests (%zu/%zu)", tests.count, tests.count);
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
        if(!record_tests(argc, argv)) return 1;
    } else if (strcmp(subcmd, "replay") == 0) {
        if(!run_tests(argc, argv)) return 1;
    } else {
        nob_log(NOB_ERROR, "Unknown subcommand: %s", subcmd);
        return 1;
    }
    return 0;
}
