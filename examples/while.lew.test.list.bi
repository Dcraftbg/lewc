:i count 4
:b shell 52
./bin/lewc examples/while.lew -o ./int/tests/while.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 45
as ./int/tests/while.s -o ./int/tests/while.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/while.o -o ./bin/tests/while
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/while
:i returncode 0
:b stdout 12
Hello World

:b stderr 0

