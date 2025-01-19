:i count 4
:b shell 48
./bin/lewc examples/sum.lew -o ./int/tests/sum.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 41
as ./int/tests/sum.s -o ./int/tests/sum.o
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

