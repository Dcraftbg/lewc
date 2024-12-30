:i count 21
:b shell 51
./bin/lewc examples/sum.lew -o ./int/tests/sum.nasm
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 55
nasm -f elf64 ./int/tests/sum.nasm -o ./int/tests/sum.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 57
gcc ./examples/sum.c ./int/tests/sum.o -o ./bin/tests/sum
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 15
./bin/tests/sum
:i returncode 0
:b stdout 14
sum(4,5) => 9

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 55
./bin/lewc examples/hello.lew -o ./int/tests/hello.nasm
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 59
nasm -f elf64 ./int/tests/hello.nasm -o ./int/tests/hello.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/hello.o -o ./bin/tests/hello
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/hello
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 65
./bin/lewc examples/func_calls.lew -o ./int/tests/func_calls.nasm
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 69
nasm -f elf64 ./int/tests/func_calls.nasm -o ./int/tests/func_calls.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 62
gcc -static ./int/tests/func_calls.o -o ./bin/tests/func_calls
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 22
./bin/tests/func_calls
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 69
./bin/lewc examples/int_literals.lew -o ./int/tests/int_literals.nasm
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 73
nasm -f elf64 ./int/tests/int_literals.nasm -o ./int/tests/int_literals.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 84
gcc ./examples/int_literals.c ./int/tests/int_literals.o -o ./bin/tests/int_literals
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 24
./bin/tests/int_literals
:i returncode 0
:b stdout 15
get_five() = 5

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 67
./bin/lewc examples/syntactical.lew -o ./int/tests/syntactical.nasm
:i returncode 1
:b stdout 0

:b stderr 35
Unknown variable or function `foo`

