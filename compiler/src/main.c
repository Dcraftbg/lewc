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
#include "compile.h"

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

static Platform default_platform = 
#if defined(_WIN32)
  OS_WINDOWS
#elif defined(__linux__)
  OS_LINUX
#endif
;
// TODO: Default arch detection
static const Architecture default_arch = ARCH_X86_64;
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
        else if (strcmp(arg, "--experimental-windows") == 0) {
            build_options.experimental_windows = true;
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

    Lexer lexer;
    lexer_create(&lexer, build_options.ipath, &atom_table, &arena);

    Parser parser = {0};
    parser_create(&parser, &lexer, &arena);
    parse(&parser, &lexer, &arena);
    
    Build build={0};
    build.path = build_options.ipath;
    build_build(&build, &parser);

    Target target={0};
    target.opath = build_options.opath;
    target.platform = default_platform;
    target.arch = default_arch;
    compile(&build, &target, &arena);

    lexer_cleanup(parser.lexer);
}
