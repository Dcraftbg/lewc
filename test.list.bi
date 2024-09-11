:i count 15
:b shell 56
./bin/compiler examples/sum.prot -o ./int/tests/sum.nasm
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

:b shell 60
./bin/compiler examples/hello.prot -o ./int/tests/hello.nasm
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
:i returncode 1
:b stdout 13
Hello World!

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 74
./bin/compiler examples/int_literals.prot -o ./int/tests/int_literals.nasm
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

