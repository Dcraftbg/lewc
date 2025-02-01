// TODO: test.c as a testing mechanism. 
// Essentially it will store command behaviour in a tests.csv file where it would have:
// # Command Count | Commands | "Expected Stdout" | "Expected Stderr" | "Provided Stdin"
// So for example you'd have something like:
// 4 | $COMPILER ./examples/sum.prot -o ./bin/tests/sum.nasm | nasm -fwin64 ./bin/tests/sum.nasm -o ./bin/tests/sum.obj | gcc ./examples/sum_main.c ./bin/tests/sum.obj -o ./bin/tests/sum | ./bin/tests/sum | "sum(3, 2) => 5" | 
// You'd probably need to use subprocess.h or something similar for capturing the stdin, stdout and stderr 
#include <stdio.h>
#include <stdlib.h>
#define NOB_IMPLEMENTATION
#include "nob.h"
#include <stdint.h>
#define CC "gcc"
#define CFLAGS "-g", "-Werror", "-Wno-unused-function", "-Wall", "-MMD", "-MP"
#define LDFLAGS "-g"

const char* get_ext(const char* path) {
    const char* end = path;
    while(*end) end++;
    while(end >= path) {
        if(*end == '.') return end+1;
        if(*end == '/' || *end == '\\') break;
        end--;
    }
    return NULL;
}
const char* get_base(const char* path) {
    const char* end = path;
    while(*end) end++;
    while(end >= path) {
        if(*end == '/' || *end == '\\') return end+1;
        end--;
    }
    return end;
}
char* shift_args(int *argc, char ***argv) {
    if((*argc) <= 0) return NULL;
    char* arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}
const char* strip_prefix(const char* str, const char* prefix) {
     size_t len = strlen(prefix);
     if(strncmp(str, prefix, strlen(prefix))==0) return str+len;
     return NULL;
}
bool nob_mkdir_if_not_exists_silent(const char *path) {
     if(nob_file_exists(path)) return true;
     return nob_mkdir_if_not_exists(path);
}
bool make_build_dirs() {
    if(!nob_mkdir_if_not_exists_silent("./bin"         )) return false;
    if(!nob_mkdir_if_not_exists_silent("./int"         )) return false;
    if(!nob_mkdir_if_not_exists_silent("./int/lewc")) return false;
    if(!nob_mkdir_if_not_exists_silent("./int/testsys" )) return false;
    return true;
}
bool remove_objs(const char* dirpath) {
   DIR *dir = opendir(dirpath);
   if (dir == NULL) {
       nob_log(NOB_ERROR, "Could not open directory %s: %s",dirpath,strerror(errno));
       return false;
   }
   errno = 0;
   struct dirent *ent = readdir(dir);
   while(ent != NULL) {
        const char* fext = get_ext(ent->d_name);
        const char* path = nob_temp_sprintf("%s/%s",dirpath,ent->d_name); 
        Nob_File_Type type = nob_get_file_type(path);
        if(strcmp(fext, "o")==0) {
            if(type == NOB_FILE_REGULAR) {
               if(!remove(path)) {
                  closedir(dir);
                  return false;
               }
            }
        }
        if (type == NOB_FILE_DIRECTORY) {
            Nob_String_Builder sb = {0};
            nob_sb_append_cstr(&sb, path);
            nob_sb_append_null(&sb);
            if(!remove_objs(sb.items)) {
                nob_sb_free(sb);
                return false;
            }
            nob_sb_free(sb);
        }
        ent = readdir(dir);
   }
   if (dir) closedir(dir);
   return true;
}

void cleanup_d(const char* obj) {
    size_t temp = nob_temp_save();
    char* str = nob_temp_strdup(obj);
    size_t str_len = strlen(str);
    assert(str_len);
    str[str_len-1] = 'd';
    remove(str);
    nob_temp_rewind(temp);
}
// TODO: cc but async
bool cc(const char* ipath, const char* opath) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, CC);
    nob_cmd_append(&cmd, CFLAGS);
    nob_cmd_append(&cmd, "-c", ipath, "-o", opath);
    if(!nob_cmd_run_sync(cmd)) {
       nob_cmd_free(cmd);
       cleanup_d(opath);
       return false;
    }
    nob_cmd_free(cmd);
    return true;
}
// TODO: nasm but async
bool nasm(const char* ipath, const char* opath) {
    Nob_Cmd cmd = {0};
    const char* end = get_base(ipath);
    nob_cmd_append(&cmd, "nasm");
    if(end) {
        Nob_String_View sv = {0};
        sv.data = ipath;
        sv.count = end-ipath;
        nob_cmd_append(&cmd, "-I", nob_temp_sv_to_cstr(sv));
    }
    nob_cmd_append(&cmd, "-f", "elf64", ipath, "-o", opath);
    if(!nob_cmd_run_sync(cmd)) {
       nob_cmd_free(cmd);
       return false;
    }
    nob_cmd_free(cmd);
    return true;
}
const char* strltrim(const char* data) {
    while(data[0] && isspace(data[0])) data++;
    return data;
}
void remove_backslashes(char* data) {
    char* backslash;
    // NOTE: Assumes strchr returns NULL on not found
    while((backslash=strchr(data, '\\'))) {
        switch(backslash[1]) {
        case '\n':
            memmove(backslash, backslash+2, strlen(backslash+2)+1);
            break;
        default:
            memmove(backslash, backslash+1, strlen(backslash+1)+1);
        }
        data=backslash;
    }
}
bool dep_analyse_str(char* data, char** result, Nob_File_Paths* paths) {
    // NOTE: Assumes strchr returns NULL on not found
    char* result_end = strchr(data, ':');
    if(!result_end) return false;
    result_end[0] = '\0';
    *result = data;
    data = result_end+1;
    remove_backslashes(data);
    char* lineend;
    if((lineend=strchr(data, '\n')))
        lineend[0] = '\0'; // Ignore all the stuff after the newline
    while((data=(char*)strltrim(data))[0]) {
        char* path=data;
        while(data[0] && data[0] != ' ') data++;
        nob_da_append(paths, path);
        if(data[0]) {
            data[0] = '\0';
            data++;
        }
    }
    return true;
}
bool _build_dir(const char* rootdir, const char* build_dir, const char* srcdir, bool forced) {
   bool result = true;
   Nob_String_Builder opath = {0};
   DIR *dir = opendir(srcdir);
   if (dir == NULL) {
       nob_log(NOB_ERROR, "Could not open directory %s: %s",srcdir,strerror(errno));
       return false;
   }
   errno = 0;
   struct dirent *ent = readdir(dir);
   while(ent != NULL) {
        const char* fext = get_ext(ent->d_name);
        const char* path = nob_temp_sprintf("%s/%s",srcdir,ent->d_name); 
        Nob_File_Type type = nob_get_file_type(path);

        if(type == NOB_FILE_REGULAR) {
           if(strcmp(fext, "c") == 0) {
               opath.count = 0;
               nob_sb_append_cstr(&opath, build_dir);
               nob_sb_append_cstr(&opath, "/");
               const char* file = strip_prefix(path, rootdir)+1;
               Nob_String_View sv = nob_sv_from_cstr(file);
               sv.count-=2; // Remove .c
               nob_sb_append_buf(&opath,sv.data,sv.count);
               nob_sb_append_cstr(&opath, ".d");
               nob_sb_append_null(&opath);
               if((!nob_file_exists(opath.items)) || nob_needs_rebuild1(opath.items, path) || forced) {
                   opath.items[opath.count-2] = 'o';
                   if(!cc(path, opath.items)) nob_return_defer(false);
               } else {
                   Nob_String_Builder dep_sb={0};
                   if(!nob_read_entire_file(opath.items, &dep_sb)) {
                       nob_return_defer(false);
                   }
                   nob_sb_append_null(&dep_sb);
                   Nob_File_Paths dep_paths={0};
                   char* obj=NULL;
                   if(!dep_analyse_str(dep_sb.items, &obj, &dep_paths)) {
                       nob_sb_free(dep_sb);
                       nob_da_free(dep_paths);
                       nob_return_defer(false);
                   }
                   if(nob_needs_rebuild(opath.items, dep_paths.items, dep_paths.count)) {
                       if(!cc(path, obj)) {
                           nob_sb_free(dep_sb);
                           nob_da_free(dep_paths);
                           nob_return_defer(false);
                       }
                   }
                   nob_sb_free(dep_sb);
                   nob_da_free(dep_paths);
               }
           } else if(strcmp(fext, "nasm") == 0) {
               opath.count = 0;
               nob_sb_append_cstr(&opath, build_dir);
               nob_sb_append_cstr(&opath, "/");
               const char* file = strip_prefix(path, rootdir)+1;
               Nob_String_View sv = nob_sv_from_cstr(file);
               sv.count-=5; // Remove .nasm
               nob_sb_append_buf(&opath,sv.data,sv.count);
               nob_sb_append_cstr(&opath, ".o");
               nob_sb_append_null(&opath);
               if((!nob_file_exists(opath.items)) || nob_needs_rebuild1(opath.items,path) || forced) {
                   if(!nasm(path,opath.items)) nob_return_defer(false);
               }
           }
        }
        if (type == NOB_FILE_DIRECTORY && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
           opath.count = 0;
           nob_sb_append_cstr(&opath, build_dir);
           nob_sb_append_cstr(&opath, "/");
           nob_sb_append_cstr(&opath, strip_prefix(path, rootdir)+1);
           nob_sb_append_null(&opath);
           if(!nob_mkdir_if_not_exists_silent(opath.items)) nob_return_defer(false);
           opath.count = 0;
           nob_sb_append_cstr(&opath, path);
           nob_sb_append_null(&opath);
           if(!_build_dir(rootdir, build_dir, opath.items, forced)) nob_return_defer(false);
        }
        ent = readdir(dir);
   }
defer:
   if (dir) closedir(dir);
   if (opath.items) nob_sb_free(opath);
   return result;
}

static bool build_dir(const char* rootdir, const char* build_dir, bool forced) {
   return _build_dir(rootdir, build_dir, rootdir, forced);
}
bool build_lewc(bool forced) {
    if(!build_dir("./src"  , "./int/lewc", forced)) return false;
    nob_log(NOB_INFO, "Built lewc successfully");
    return true;
}

bool compile_testsys(bool forced) {
    if(!build_dir("./testsys/src"  , "./int/testsys", forced)) return false;
    nob_log(NOB_INFO, "Built testsys successfully");
    return true;
}
bool find_objs(const char* dirpath, Nob_File_Paths *paths) {
    Nob_String_Builder sb={0};
    bool result = true;
    DIR *dir = NULL;

    dir = opendir(dirpath);
    if (dir == NULL) {
        nob_log(NOB_ERROR, "Could not open directory %s: %s", dirpath, strerror(errno));
        nob_return_defer(false);
    }

    errno = 0;
    struct dirent *ent = readdir(dir);
    while (ent != NULL) {
        const char* ent_d_name = nob_temp_strdup(ent->d_name);
        const char* fext = get_ext(ent_d_name);
        const char* path = nob_temp_sprintf("%s/%s",dirpath,ent_d_name);
        Nob_File_Type type = nob_get_file_type(path);
        
        if(fext && strcmp(fext, "o") == 0) {
            if(type == NOB_FILE_REGULAR) {
                nob_da_append(paths,path);
            }
        }
        if (type == NOB_FILE_DIRECTORY) {
            if(strcmp(ent_d_name, ".") != 0 && strcmp(ent_d_name, "..") != 0) {
                sb.count = 0;
                nob_sb_append_cstr(&sb,nob_temp_sprintf("%s/%s",dirpath,ent_d_name));
                nob_sb_append_null(&sb);
                if(!find_objs(sb.items, paths)) nob_return_defer(false);
            }
        }
        ent = readdir(dir);
    }

    if (errno != 0) {
        nob_log(NOB_ERROR, "Could not read directory %s: %s", dirpath, strerror(errno));
        nob_return_defer(false);
    }

defer:
    if (dir) closedir(dir);
    nob_sb_free(sb);
    return result;
}
bool ld(Nob_File_Paths* paths, const char* opath) {
    nob_log(NOB_INFO, "Linking %s",opath);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, CC);
#ifdef LDFLAGS
    nob_cmd_append(&cmd, LDFLAGS);
#endif
    nob_cmd_append(&cmd, "-o", opath);
    for(size_t i = 0; i < paths->count; ++i) {
        nob_cmd_append(&cmd, paths->items[i]);
    }
    if(!nob_cmd_run_sync(cmd)) {
        nob_cmd_free(cmd);
        return false;
    }
    nob_cmd_free(cmd);
    nob_log(NOB_INFO, "Linked %s successfully", opath);
    return true;
}
bool link_lewc() {
    nob_log(NOB_INFO, "Linking lewc");
    Nob_File_Paths paths = {0};
    if(!find_objs("./int/lewc",&paths)) {
        return false;
    }
    if(!ld(&paths, "./bin/lewc")) {
        nob_da_free(paths);
        return false;
    }
    nob_da_free(paths);
    nob_log(NOB_INFO, "Linked lewc successfully");
    return true;
}


bool link_testsys() {
    nob_log(NOB_INFO, "Linking testsys");
    Nob_File_Paths paths = {0};
    if(!find_objs("./int/testsys",&paths)) {
        return false;
    }
    if(!ld(&paths, "./bin/testsys")) {
        nob_da_free(paths);
        return false;
    }
    nob_da_free(paths);
    nob_log(NOB_INFO, "Linked testsys successfully");
    return true;
}
typedef struct {
    char* exe;
    int argc;
    char** argv;
} Build;
typedef struct {
   const char* name;
   bool (*run)(Build*);
   const char* desc;
} Cmd;
bool help(Build* build);
bool build(Build* build);
bool run(Build* build);
bool bruh(Build* build);
bool test(Build* build);

Cmd commands[] = {
   { .name = "help"       , .run=help       , .desc="Help command that explains either what a specific subcommand does or lists all subcommands" },
   { .name = "build"      , .run=build      , .desc="Build to lewc" },
   { .name = "run"        , .run=run        , .desc="Run the lewc" },
   { .name = "bruh"       , .run=bruh       , .desc="Build+Run the lewc" },
   { .name = "test"       , .run=test       , .desc="Run test suite or test command" },
};

bool help(Build* build) {
    const char* what = shift_args(&build->argc, &build->argv);
    if(what) {
        for(size_t i = 0; i < NOB_ARRAY_LEN(commands); ++i) {
             if(strcmp(commands[i].name, what) == 0) {
                 nob_log(NOB_INFO, "%s: %s",what,commands[i].desc);
                 return true; 
             }
        }
        nob_log(NOB_ERROR, "Unknown subcommand: %s",what);
        return false;
    }
    nob_log(NOB_INFO, "%s <subcommand>", build->exe);
    nob_log(NOB_INFO, "List of subcommands:");
    for(size_t i = 0; i < NOB_ARRAY_LEN(commands); ++i) {
        nob_log(NOB_INFO, "  %s: %s",commands[i].name,commands[i].desc);
    }
    return true;
}
bool build(Build* build) {
    if(!make_build_dirs()) return false;
    bool forced = false;
    if(build->argc > 0 && strcmp(build->argv[0], "-f")==0) {
        forced = true;
        shift_args(&build->argc, &build->argv);
    }
    if(!build_lewc(forced)) return false;
    if(!link_lewc()) return false;
    return true;
}
bool run(Build* build) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(
        &cmd,
        "./bin/lewc"
    );
    nob_da_append_many(&cmd, build->argv, build->argc);
    if (!nob_cmd_run_sync(cmd)) {
        nob_cmd_free(cmd);
        return false;
    }
    nob_cmd_free(cmd);
    return true;
}

bool run_testsys(Build* build) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(
        &cmd,
        "./bin/testsys"
    );
    nob_da_append_many(&cmd, build->argv, build->argc);
    if (!nob_cmd_run_sync(cmd)) {
        nob_cmd_free(cmd);
        return false;
    }
    nob_cmd_free(cmd);
    return true;
}


bool bruh(Build* b) {
    if(!build(b)) return false;
    if(!run(b)) return false;
    return true;
}

bool build_testsys(Build* build) {
    if(!make_build_dirs()) return false;
    bool forced = false;
    if(build->argc > 0 && strcmp(build->argv[0], "-f")==0) {
        forced = true;
        shift_args(&build->argc, &build->argv);
    }
    if(!compile_testsys(forced)) return false;
    if(!link_testsys()) return false;
    return true;
}
bool test(Build* b) {
    if(!build(b)) return false;
    if(!build_testsys(b)) return false; 
    if(!run_testsys(b)) return false;
    return true;
}
int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc,argv);
    Build build = {0};
    build.exe = shift_args(&argc, &argv);
    assert(build.exe && "First argument should be program itself");
    const char* cmd = shift_args(&argc, &argv);
    build.argc = argc;
    build.argv = argv;
    if(cmd == NULL) {
        nob_log(NOB_ERROR, "Expected subcommand but found nothing!");
        help(&build);
        return 1;
    }
    for(size_t i = 0; i < NOB_ARRAY_LEN(commands); ++i) {
        if(strcmp(commands[i].name,cmd) == 0) {
            if(!commands[i].run(&build)) return 1;
            return 0;
        }
    }
    nob_log(NOB_ERROR, "Unknown subcommand %s", cmd);
    return 1;
}
