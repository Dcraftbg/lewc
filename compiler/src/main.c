#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "list.h"
#include "arena.h"
#include "atom.h"
#include "utils.h"
#include "fileutils.h"
#include "utf8.h"
#include "token.h"
#include "lexer.h"
#include "type.h"
#include "symbol.h"
#include "ast.h"
#include "parser.h"
#include "build.h"

const char* shift_args(int *argc, const char ***argv) {
    if((*argc) <= 0) return NULL;
    const char* arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}

BuildOptions build_options = { 0 };
#define UPRINTF(...) fprintf(stderr, __VA_ARGS__)
void usage() {
    UPRINTF("Usage %s:\n",build_options.exe);
    UPRINTF(" -o <path> - Specify output path\n");
    UPRINTF(" <path>    - Specify input path\n");
}
typedef enum {
    OUTPUT_NASM,

    OUTPUT_COUNT
} OutputKind;
typedef enum {
    ARCH_X86_64,

    ARCH_COUNT
} Architecture;
typedef enum {
    OS_WINDOWS,

    PLATFORM_COUNT
} Platform;

typedef struct {
    const char* opath;
    OutputKind outputKind;
    Architecture arch;
    Platform platform;
} Target;
static_assert(OUTPUT_COUNT == 1, "Update outputkind_map");
const char* outputkind_map[OUTPUT_COUNT] = {
    "Nasm Assembly" 
};
const char* outputkind_str(OutputKind kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(outputkind_map));
    return outputkind_map[kind];
}

static_assert(ARCH_COUNT == 1, "Update arch_map");
const char* arch_map[ARCH_COUNT] = {
    "x86_64" 
};
const char* arch_str(Architecture kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(arch_map));
    return arch_map[kind];
}


static_assert(PLATFORM_COUNT == 1, "Update platform_map");
const char* platform_map[PLATFORM_COUNT] = {
    "Windows" 
};
const char* platform_str(Platform kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(platform_map));
    return platform_map[kind];
}


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
#define nasm_gpr_reset(alloc) memset(&(alloc)->bitmap, 0, sizeof((alloc)->bitmap))


typedef struct {
    Build* build;
    Target* target;
    FILE* f;
    Arena* arena;
    NasmGPRegsAlloc regs;
} CompileState;
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

enum {
    CVALUE_REGISTER
};

typedef struct {
    int kind;
    union {
        struct { size_t regsize; size_t reg; };
    };
} CompileValue;
int WINDOWS_GPR_ARGS[] = {
    REG_C,
    REG_D,
    REG_R8,
    REG_R9
};
CompileValue compile_value_alloc(CompileState* state, size_t size) {
    return (CompileValue){.kind=CVALUE_REGISTER, .reg=nasm_gpr_alloc(&state->regs), .regsize=size};
}
void compile_nasm_x86_64_windows(CompileState* state) {
    state->f = fopen(state->target->opath, "wb");
    if(!state->f) {
        eprintfln("ERROR: Could not open output file %s: %s",state->target->opath,strerror(errno));
        exit(1);
    }
    nprintfln("[BITS 64]");
    nprintfln("; Generated by the C prototype compiler");
    nprintfln("; Target: %s %s",platform_str(state->target->platform), arch_str(state->target->arch));
    nprintfln("section .text");
    for(size_t i = 0; i < state->build->funcs.len; ++i) {
        nasm_gpr_reset(&state->regs);
        BuildFunc* func = &state->build->funcs.items[i];
        Type* type = type_table_get(&state->build->type_table, func->typeid);
        assert(type);
        assert(type->core == CORE_FUNC);
        FuncSignature* sig = &type->signature;

        for(size_t j = 0; j < sig->input.len && j < ARRAY_LEN(WINDOWS_GPR_ARGS); ++j) {
            nasm_gpr_occupy(&state->regs, WINDOWS_GPR_ARGS[j]);
        }
        nprintfln("global %s",func->name->data);
        nprintfln("%s:",func->name->data);
        for(size_t j = 0; j < func->blocks.len; ++j) {
           
            // TODO: Deallocate this at the end since its at the top of the arena
            Block* block = &func->blocks.items[j];
            CompileValue* vals = (CompileValue*)arena_alloc(state->arena, sizeof(CompileValue)*block->len);
            if(!vals) {
                eprintfln("ERROR: Failed to allocate temporary vals buffer");
                exit(1);
            }
            memset(vals, 0, sizeof(CompileValue)*block->len);
            nprintfln(".%zu:",j);
            for(size_t k = 0; k < block->len; ++k) {
                BuildInst* inst = &block->items[k];
                static_assert(BUILD_INST_COUNT == 3);
                switch(inst->kind) {
                case BUILD_LOAD_ARG: {
                    assert(inst->arg < sig->input.len);
                    Arg* arg = &sig->input.items[inst->arg];
                    Type* argtype = type_table_get(&state->build->type_table, arg->typeid);
                    switch(inst->arg) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                       size_t regsize = 0;
                       if(argtype->core != CORE_I32) {
                           eprintfln("TODO: Core type: %d is not implemented", argtype->core);
                           exit(1);
                       }
                       regsize = REG_SIZE_32;
                       vals[k] = (CompileValue){.kind=CVALUE_REGISTER, .reg=WINDOWS_GPR_ARGS[inst->arg], .regsize=regsize};
                       break; 
                    default:
                       eprintfln("Argument %zu is not implemented",inst->arg);
                       exit(1);
                    }
                } break;
                case BUILD_ADD_INT_SIGNED: {
                    assert(inst->v0 < k && inst->v1 < k); // It is a previous instruction.
                    CompileValue* v0 = &vals[inst->v0];
                    CompileValue* v1 = &vals[inst->v1];

                    assert(v0->kind == CVALUE_REGISTER);
                    assert(v1->kind == CVALUE_REGISTER);
                    assert(v0->regsize == v1->regsize);
                    CompileValue  result = compile_value_alloc(state, v0->regsize);
                    assert(result.kind == CVALUE_REGISTER);

                    nprintfln("   mov %s, %s", nasm_gpr_to_str(result.reg, result.regsize), nasm_gpr_to_str(v0->reg   , v0->regsize   ));
                    nprintfln("   add %s, %s", nasm_gpr_to_str(result.reg, result.regsize), nasm_gpr_to_str(v1->reg   , v1->regsize   ));
                    vals[k] = result;
                } break;
                case BUILD_RETURN: {
                    assert(inst->arg < k); // It is a previous instruction.
                    CompileValue value = {0};
                    CompileValue* arg = &vals[inst->arg];
                    if(arg->kind == CVALUE_REGISTER && arg->reg == REG_A) {
                        nprintfln("   ret");
                        break;
                    }
                    assert(arg->kind == CVALUE_REGISTER);
                    value.reg = REG_A;
                    value.regsize = arg->regsize;
                    nprintfln("   mov %s, %s",nasm_gpr_to_str(value.reg, value.regsize), nasm_gpr_to_str(arg->reg, arg->regsize));
                    nprintfln("   ret");
                } break;
                default:
                    eprintfln("Unhandled instruction %d", inst->kind);
                    exit(1);
                }
            }
        }
    }
    fclose(state->f);
}
void compile(Build* build, Target* target, Arena* arena) {
    static_assert(OUTPUT_COUNT == 1, "Update compile");
    CompileState state = {0};
    state.target = target;
    state.build = build;
    state.arena = arena;
    if(target->arch == ARCH_X86_64 && target->platform == OS_WINDOWS) {
        compile_nasm_x86_64_windows(&state);
    } else 
    {
        eprintfln("Unsupported target %s:%s outputing to %s", platform_str(target->platform), arch_str(target->arch), outputkind_str(target->outputKind));
        exit(1);
    }
}
int main(int argc, const char** argv) {
    build_options.exe = shift_args(&argc, &argv);
    assert(build_options.exe);
    const char* arg = NULL;
    while ((arg = shift_args(&argc, &argv))) {
        if (build_options.ipath == NULL) build_options.ipath = arg;
        else if (strcmp(arg, "-o") == 0) {
            if (build_options.opath) {
                fprintf(stderr, "Output path already specified!\n");
                usage();
                exit(1);
            }
            build_options.opath = shift_args(&argc, &argv);
            if (!build_options.opath) {
                fprintf(stderr, "Expected output path after -o but found nothing!\n");
                usage();
                exit(1);
            }
        }
        else {
            fprintf(stderr, "Unknown argument: '%s'\n", arg);
            usage();
            exit(1);
        }
    }
    if(!build_options.ipath) {
        eprintf("ERROR: Missing input path!\n");
        usage();
        exit(1);
    }
    const char* default_opath = "out.nasm";
    if(!build_options.opath) {
        eprintf("WARN: No output path specified, outputting to: %s\n", default_opath);
        build_options.opath = default_opath;
    }
    Arena arena = {0};
    AtomTable atom_table={0};
    atom_table.arena = &arena;

    Lexer lexer = lexer_create(build_options.ipath, &atom_table);

    Parser parser = {0};
    parser_create(&parser, &lexer, &arena);
    parse(&parser, &lexer, &arena);
    
    Build build={0};
    build_build(&build, &parser);

    Target target={0};
    target.opath = build_options.opath;
    target.platform = OS_WINDOWS;
    target.arch = ARCH_X86_64;
    compile(&build, &target, &arena);

    lexer_cleanup(parser.lexer);
}
