#include "nasm.h"
#include <stdio.h>

const char* NASM_GPR_U8_MAP [REG_GPR_COUNT] = {
    "al",
    "bl",
    "cl",
    "dl",
    "dll",
    "sll",
    "r8b",
    "r9b",
};
const char* NASM_GPR_U16_MAP [REG_GPR_COUNT] = {
    "ax",
    "bx",
    "cx",
    "dx",
    "di",
    "si",
    "r8w",
    "r9w",
};
const char* NASM_GPR_U32_MAP [REG_GPR_COUNT] = {
    "eax",
    "ebx",
    "ecx",
    "edx",
    "edi",
    "esi",
    "r8d",
    "r9d",
};
const char* NASM_GPR_U64_MAP [REG_GPR_COUNT] = {
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "rdi",
    "rsi",
    "r8" ,
    "r9" ,
};

void nasm_gpr_occupy(NasmGPRegsAlloc* alloc, size_t reg) {
    assert(reg < REG_GPR_COUNT);
    alloc->bitmap[reg / 8] |= (1<<(reg%8));
}
size_t nasm_gpr_alloc(NasmGPRegsAlloc* alloc) {
    for(size_t i = 0; i < BITMAP_BYTES(REG_GPR_COUNT); ++i) {
        if(alloc->bitmap[i] == 0xFF) continue;
        for(size_t j = 0; j < 8; ++j) {
            if((alloc->bitmap[i] & (1<<j)) == 0) {
                alloc->bitmap[i] |= 1<<j;
                return i*8 + j;
            }
        }
    }
    return INVALID_REG;
}
bool nasm_gpr_is_free(NasmGPRegsAlloc* alloc, size_t reg) {
    assert(reg < REG_GPR_COUNT);
    return alloc->bitmap[reg / 8] & (1 << reg%8);
}
const char* nasm_gpr_to_str(int reg, int regsize) {
    static_assert(REG_SIZE_COUNT == 4, "Update nasm_gpr_to_str");
    switch(regsize) {
    case REG_SIZE_8:
        return NASM_GPR_U8_MAP[reg];
    case REG_SIZE_16:
        return NASM_GPR_U16_MAP[reg];
    case REG_SIZE_32:
        return NASM_GPR_U32_MAP[reg];
    case REG_SIZE_64:
        return NASM_GPR_U64_MAP[reg];
    default:
        eprintfln("Unhandled gpr size: %d",regsize);
        exit(1);
    }
}
