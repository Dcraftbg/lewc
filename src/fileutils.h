#pragma once
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define FS_DEALLOC(data, size) free(data)
#define FS_MALLOC(size) malloc(size)
// TODO: Utf8 checks
const char* read_entire_file(const char* path, size_t* size);
// NOTE: Stolen from nob.h
const char *path_name(const char *path);
