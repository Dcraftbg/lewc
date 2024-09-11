#pragma once
#include "../../compile.h"
#include <stddef.h>
#include "../../type.h"
#include <stdlib.h>
#include <stdio.h>
#include "../../utils.h"

enum {
    CVALUE_REGISTER,
    CVALUE_STACK_PTR,
    CVALUE_CONST_INT,
};
typedef struct {
    int kind;
    union {
        struct { size_t regsize  ; size_t reg   ; };
        struct { size_t stack_ptr; typeid_t type; };
        struct { uint64_t value  ; typeid_t type; } integer;
    };
} CompileValue;

#define nprintf(...) do { \
       if(fprintf(state->f, __VA_ARGS__) < 0) {\
            eprintfln("ERROR: Failed to encode something");\
            fclose(state->f);\
            exit(1);\
       }\
   } while(0)
#define nprintfln(...) do {\
       nprintf(__VA_ARGS__);\
       fputs(NEWLINE, state->f);\
   } while(0)



static CompileValue compile_value_alloc(CompileState* state, size_t size) {
    return (CompileValue){.kind=CVALUE_REGISTER, .reg=nasm_gpr_alloc(&state->regs), .regsize=size};
}
