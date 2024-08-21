:i count 5
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

