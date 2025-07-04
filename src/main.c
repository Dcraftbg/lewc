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
#include "ast.h"
#include "parser.h"
#include "compile.h"
#include "strutils.h"
#include "build.h"
#include "version.h"
#include "darray.h"

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
    UPRINTF("Usage %s: (v"VERSION_STR" "VERSION_STABLE")\n", build_options.exe);
    UPRINTF(" -o <path>             - Specify output path\n");
    UPRINTF(" <path>                - Specify input path\n");
    UPRINTF(" --arch=<arch>         - Specify output architecture [x86_64]\n");
    UPRINTF(" --platform=<platform> - Specify output platform [Linux]\n");
    UPRINTF(" --backend=<backend>   - Specify backend [qbe]\n");
    UPRINTF(" --okind=<output kind> - Specify output kind [ir, [gas, s, asm], [obj, o]]\n");
    UPRINTF(" -I<include directory> - Specify include directory\n");
    UPRINTF(" -v|--version          - Get lewc version\n");
    // UPRINTF("Assembler: `"LEW_ASSEMBLER"`\n");
}

static Platform default_platform = 
#if defined(_WIN32)
  OS_WINDOWS
#elif defined(__linux__)
  OS_LINUX
#else
  OS_UNDEFINED
// # warning Your platform is not supported as a default_platform. Youll likely have to specify it explicitly
#endif
;
// TODO: Default arch detection
static const Architecture default_arch = ARCH_X86_64;
static const Backend default_backend = BACKEND_QBE;
int main(int argc, const char** argv) {
    Target target={0};
    target.platform   = default_platform;
    target.arch       = default_arch;
    target.backend    = default_backend;
    target.outputKind = OUTPUT_OBJ;

    build_options.exe = shift_args(&argc, &argv);
    assert(build_options.exe);
    const char* arg      = NULL;
    const char* arch     = NULL;
    const char* platform = NULL;
    const char* backend  = NULL;
    const char* okind    = NULL;
    const char* includedir = NULL;
    while ((arg = shift_args(&argc, &argv))) {
        if (strcmp(arg, "-o") == 0) {
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
        else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            fprintf(stderr, "lewc v"VERSION_STR" "VERSION_STABLE"\n");
            return 0;
        }
        else if ((arch=strstrip(arg, "--arch="))) {
            if(strcmp(arch, "x86_64")==0) target.arch = ARCH_X86_64;
            else {
                eprintfln("ERROR Unknown target arch: %s", arch);
                exit(1);
            }
        }
        else if ((platform=strstrip(arg, "--platform="))) {
            if (strcmp(platform, "Linux")==0) target.platform = OS_LINUX;
            else {
                eprintfln("ERROR Unknown target platform: %s", platform);
                exit(1);
            }
        }
        else if ((backend=strstrip(arg, "--backend="))) {
            if (strcmp(backend, "qbe") == 0) target.backend = BACKEND_QBE;
            else {
                eprintfln("ERROR Unknown target backend: %s", backend);
                exit(1);
            }
        }
        else if ((okind=strstrip(arg, "--okind="))) {
                 if (strcmp(okind, "obj") == 0 || strcmp(okind, "o") == 0) target.outputKind = OUTPUT_OBJ;
            else if (strcmp(okind, "asm") == 0 || strcmp(okind, "gas") == 0 || strcmp(okind, "s") == 0) target.outputKind = OUTPUT_GAS;
            else if (strcmp(okind, "ir") == 0) target.outputKind = OUTPUT_IR;
            else {
                eprintfln("ERROR Unknown output kind: %s", okind);
            }
        }
        else if ((includedir=strstrip(arg, "-I"))) {
            da_push(&build_options.includedirs, includedir);
        }
        else {
            if (build_options.ipath == NULL) {
                build_options.ipath = arg;
            } else {
                fprintf(stderr, "Unknown argument: '%s'\n", arg);
                usage();
                exit(1);
            }
        }
    }
    if(!build_options.ipath) {
        eprintf("ERROR Missing input path!\n");
        usage();
        exit(1);
    }
    if(!target.platform) {
        eprintfln("ERROR Could not infer platform automatically. Please specify it with --platform={your platform}");
        usage();
        exit(1);
    }
    const char* default_opath = "out.s";
    if(!build_options.opath) {
        eprintf("WARN: No output path specified, outputting to: %s\n", default_opath);
        build_options.opath = default_opath;
    }
    Arena arena = {0};
    AtomTable atom_table={0};
    atom_table.arena = &arena;

    Lexer lexer;
    lexer_create(&lexer, build_options.ipath, &atom_table, &arena);

    ProgramState state = {0};
    state.arena = &arena;
    state.main = module_new(&arena, build_options.ipath);
    type_table_init(&state.main->type_table);
    Parser parser = {0};
    parser_create(&parser, &lexer, &arena, state.main);
    parse(&parser, &arena);

    if(!module_do_intermediate_steps(state.main)) exit(1);
    Build build = {0};
    build.target  = &target;
    build.options = &build_options;
    if(!build_build(&build, &state)) exit(1);
    lexer_cleanup(parser.lexer);
}
