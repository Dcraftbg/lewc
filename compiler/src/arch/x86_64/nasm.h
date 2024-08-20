#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../utils.h"

enum {
    REG_A,
    REG_B,
    REG_C,
    REG_D,
    REG_DI,
    REG_SI,
    REG_R8,
    REG_R9,
    // r1->r15?
    REG_GPR_COUNT
};

enum {
    REG_SIZE_8 ,
    REG_SIZE_16,
    REG_SIZE_32,
    REG_SIZE_64,

    REG_SIZE_COUNT
};
typedef struct {
    uint8_t bitmap[BITMAP_BYTES(REG_GPR_COUNT)];
} NasmGPRegsAlloc;

#define INVALID_REG ((size_t)-1)


void nasm_gpr_occupy(NasmGPRegsAlloc* alloc, size_t reg);
size_t nasm_gpr_alloc(NasmGPRegsAlloc* alloc);
bool nasm_gpr_is_free(NasmGPRegsAlloc* alloc, size_t reg);
const char* nasm_gpr_to_str(int reg, int regsize);
#define nasm_gpr_reset(alloc) memset(&(alloc)->bitmap, 0, sizeof((alloc)->bitmap))
