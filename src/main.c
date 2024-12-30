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
#include "build.h"
#include "compile.h"
#include "strutils.h"
#include "progstate.h"
#include "syn_analys.h"

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
    UPRINTF("Usage %s:\n", build_options.exe);
    UPRINTF(" -o <path>             - Specify output path\n");
    UPRINTF(" <path>                - Specify input path\n");
    UPRINTF(" --arch=<arch>         - Specify output architecture\n");
    UPRINTF(" --platform=<platform> - Specify output platform\n");
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
int main(int argc, const char** argv) {
    Target target={0};
    target.platform = default_platform;
    target.arch = default_arch;

    build_options.exe = shift_args(&argc, &argv);
    assert(build_options.exe);
    const char* arg      = NULL;
    const char* arch     = NULL;
    const char* platform = NULL;
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
        else if (strcmp(arg, "--experimental-windows") == 0) {
            build_options.experimental_windows = true;
        }
        else if ((arch=strstrip(arg, "--arch="))) {
            if(strcmp(arch, "x86_64")==0) target.arch = ARCH_X86_64;
            else {
                eprintfln("ERROR: Unknown target arch: %s", arch);
                exit(1);
            }
        }
        else if ((platform=strstrip(arg, "--platform="))) {
            if      (strcmp(platform, "Windows")==0) target.platform = OS_WINDOWS;
            else if (strcmp(platform, "Linux"  )==0) target.platform = OS_LINUX;
            else {
                eprintfln("ERROR: Unknown table platform: %s", platform);
                exit(1);
            }
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
        eprintf("ERROR: Missing input path!\n");
        usage();
        exit(1);
    }
    if(!target.platform) {
        eprintfln("ERROR: Could not infer platform automatically. Please specify it with --platform={your platform}");
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

    Lexer lexer;
    lexer_create(&lexer, build_options.ipath, &atom_table, &arena);

    ProgramState state = {0};
    type_table_init(&state.type_table);
    Parser parser = {0};
    parser_create(&parser, &lexer, &arena, &state);
    parse(&parser, &lexer, &arena);

    if(!syn_analyse(&state)) exit(1);
    
    Build build={0};
    build.path = build_options.ipath;
    build_build(&build, &state);

    target.opath = build_options.opath;
    compile(&build, &target, &arena);

    lexer_cleanup(parser.lexer);
}
