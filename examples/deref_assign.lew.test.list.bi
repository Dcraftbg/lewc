:i count 4
:b shell 66
./bin/lewc examples/deref_assign.lew -o ./int/tests/deref_assign.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 59
as ./int/tests/deref_assign.s -o ./int/tests/deref_assign.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 92
gcc -static ./examples/deref_assign.c ./int/tests/deref_assign.o -o ./bin/tests/deref_assign
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 24
./bin/tests/deref_assign
:i returncode 0
:b stdout 13
a = 5
a = 69

:b stderr 0

