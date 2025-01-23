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
    "syntactical.lew",
    "deref.lew",
    "nested.lew",
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
    Nob_Fd read, write;
} Nob_Pipe;
// FIXME: This leaks on child process:
//  either read for stdout/stderr
//  or    write for stdin
// Its fine. Most programs don't have anything against it and the system *should* automatically clean it up
bool nob_pipe_create(Nob_Pipe* result) {
#ifdef _WIN32
#   error Nob_Pipe isnt supported on Windows
#endif
    int pipes[2];
    if(pipe(pipes) == -1) {
        nob_log(NOB_ERROR, "Could not create pipe %s", strerror(errno));
        return false;
    }
    result->read = pipes[0];
    result->write = pipes[1];
    return true;
}
void nob_pipe_close(Nob_Pipe* pipe) {
#ifdef _WIN32
#   error Nob_Pipe isnt supported on Windows
#endif
    close(pipe->write);
    close(pipe->read);
    pipe->write = pipe->read = NOB_INVALID_FD;
}
ssize_t nob_fd_read(Nob_Fd fd, void* buf, size_t size) {
#ifdef _WIN32
#   error Nob_Pipe isnt supported on Windows
#endif
    // NOTE: read will automatically set errno
    return read(fd, buf, size);
}

typedef struct {
    const char* path;
    bool failed;
    Nob_Proc proc;
    Nob_Pipe std_pipe;
} Test;
typedef struct {
    Test* items;
    size_t count, capacity;
    size_t failed_tests;
} Tests;
bool test_create(Test* test, Nob_Cmd* cmd, const char* path) {
    test->failed = false;  
    if(!nob_pipe_create(&test->std_pipe)) return false;
    Nob_Cmd_Redirect redirect = {
        .fdout = &test->std_pipe.write,
        .fdin = NULL,
        .fderr = &test->std_pipe.write,
    };
    test->proc = nob_cmd_run_async_redirect_and_reset(cmd, redirect);
    if(test->proc == NOB_INVALID_PROC) {
        nob_log(NOB_ERROR, "Failed to spawn test `%s`", test->path);
        nob_pipe_close(&test->std_pipe);
        return false;
    }
    nob_fd_close(test->std_pipe.write);
    test->std_pipe.write = NOB_INVALID_FD;
    return true;
}
void wait_tests(Tests* tests) {
    for(size_t i = 0; i < tests->count; ++i) {
        if((tests->items[i].failed = !nob_proc_wait(tests->items[i].proc))) {
            tests->failed_tests++;
        }
    }
}
void log_tests(Tests* tests) {
    for(size_t i = 0; i < tests->count; ++i) {
        if(tests->items[i].failed) {
            nob_log(NOB_ERROR, "Test %zu (%s) failed", i, tests->items[i].path);
            ssize_t n = 0;
            char buf[4096];
            while((n=nob_fd_read(tests->items[i].std_pipe.read, buf, sizeof(buf)-1)) > 0) {
                fprintf(stderr, "%.*s", (int)n, buf);
            }
            if(n < 0) nob_log(NOB_ERROR, "ERROR: Failed reading: %s", strerror(errno));
        }
    }
    if(tests->failed_tests) {
        nob_log(NOB_ERROR, "%zu/%zu tests failed", tests->failed_tests, tests->count);
        return;
    }
    nob_log(NOB_INFO, "OK all tests passed (%zu/%zu)", tests->count, tests->count);
}
void cleanup_tests(Tests* tests) {
    for(size_t i = 0; i < tests->count; ++i) {
        nob_pipe_close(&tests->items[i].std_pipe);
    }
    nob_da_free(*tests);
}
bool log_and_cleanup_tests(Tests* tests) {
    wait_tests(tests);
    log_tests(tests);
    size_t failed_tests = tests->failed_tests;
    cleanup_tests(tests);
    return failed_tests == 0;
}
bool record_tests(int argc, const char** argv) {
    Tests tests = {0};
    Nob_Cmd cmd = {0};
    Test test;
    if(argc) {
        while((test.path=shift_args(&argc, &argv))) {
            test_record(&cmd, test.path);
            if(test_create(&test, &cmd, test.path)) {
                nob_da_append(&tests, test);
            }
        }
    } else {
        for(size_t i = 0; i < NOB_ARRAY_LEN(all_tests); ++i) {
            test.path = all_tests[i];
            test_record(&cmd, test.path);
            if(test_create(&test, &cmd, test.path)) {
                nob_da_append(&tests, test);
            }
        }
    }
    nob_da_free(cmd);
    return log_and_cleanup_tests(&tests);
}
bool run_tests(int argc, const char** argv) {
    Tests tests = {0};
    Nob_Cmd cmd = {0};
    if(argc) {
        Test test;
        while((test.path=shift_args(&argc, &argv))) {
            test_replay(&cmd, test.path);
            if(test_create(&test, &cmd, test.path)) {
                nob_da_append(&tests, test);
            }
        }
    } else {
        Test test;
        for(size_t i = 0; i < NOB_ARRAY_LEN(all_tests); ++i) {
            test.path = all_tests[i];
            test_replay(&cmd, test.path);
            if(test_create(&test, &cmd, test.path)) {
                nob_da_append(&tests, test);
            }
        }
    }
    nob_da_free(cmd);
    return log_and_cleanup_tests(&tests);
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
