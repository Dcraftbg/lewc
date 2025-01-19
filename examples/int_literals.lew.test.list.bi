:i count 4
:b shell 66
./bin/lewc examples/int_literals.lew -o ./int/tests/int_literals.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 59
as ./int/tests/int_literals.s -o ./int/tests/int_literals.o
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

