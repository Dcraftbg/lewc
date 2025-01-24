#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _WIN32
#   define NEWLINE "\r\n"
#else
#   define NEWLINE "\n"
#endif
// Inline keyword
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define INLINE inline
#else
    #define INLINE
#endif
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eprintfln(...) (eprintf(__VA_ARGS__), fputs(NEWLINE, stderr))
#define STRINGIFY1(x) # x
#define STRINGIFY2(x) STRINGIFY1(x)
#define unreachable(...) (eprintfln("ERROR:" __FILE__ ":" STRINGIFY2(__LINE__) " unreachable " __VA_ARGS__), abort())
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))
#define BITMAP_BYTES(n) ((n+7)/8)
#define BIT(n) (1<<(n-1))


/*
#if defined(__STDC_VERSION__)
#   if __STDC_VERSION__ >= 202311L
        // Since C23:
        // Do nothing, static_assert is already defined
#   elif __STDC_VERSION__ >= 201112L
#       define static_assert _Static_assert 
#   else
#       if defined(STRIP_STATIC_ASSERT)
#           define static_assert(...)
#       else 
#           error ERROR: Cannot build with this old of a version of C. Please use C11 or later
#       endif
#   endif 
#else
#   if defined(STRIP_STATIC_ASSERT)
#       define static_assert(...)
#   else
#       error ERROR: Cannot build with this old of a version of C. Please use C11 or later
#   endif
#endif
*/
