v0.5.1 alpha: (hash: 65e71f7)
- Include directories (-I flag)
v0.5 alpha: (hash: ad8018e)
- Added --okind flag
- Added --okind=ir, --okind=s/asm/gas, --okind=obj/o
- No longer output intermediate .ssa file when generating assembly
- Help for options (--okind, --arch, --platform, --backend) specifying what things you can set to them
- Support output to object file
- Make --okind=obj the default option
v0.4 alpha:
- Fix error reporting - report location in const_eval
- struct literals - struct Foo { .a = 0, .b = 1 }
- Global variables - Global variables!! Still no external ones tho :(
- Fix building of load with i/u64
- Typefix body of ifs/elze/while
- Fix ext for offseting pointer with i/u64
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
