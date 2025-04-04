v0.3 alpha: (hash: a84bbaf)
- Fix #import to find based on the file location instead of current directory
- std/ffi and std/ffi/c.lew - FFI definitions for C types
- std/libc/stddef.lew - (s)size_t, ptrdiff_t
- std/libc - Changes to use stddef.lew definitions
- c"" is now `*i8` instead of `*u8` - Changed cstrings to match `*c_char` (which is `*i8`)
- Fix error reporting - for most things errors should now print location. Report if you see any missing locations 
v0.2.1 alpha: (hash: 6db09fe)
- Fix return integer bug that causes Segfault
v0.2 alpha: (hash: cbb6a26) 
- i8, i16, i64, u64 - added integer types
- #import - import directive + update highlighting
- ""  - parsing string tokens (only used for import atm)
- std - standard library with a few bindings to libc under std/libc
