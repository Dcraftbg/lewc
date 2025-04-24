:i count 3
:b shell 50
./bin/lewc examples/void.lew -o ./int/tests/void.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 68
gcc -static ./examples/void.c ./int/tests/void.o -o ./bin/tests/void
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 16
./bin/tests/void
:i returncode 0
:b stdout 31
Hello from foo (void function)

:b stderr 0

