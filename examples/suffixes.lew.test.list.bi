:i count 4
:b shell 58
./bin/lewc examples/suffixes.lew -o ./int/tests/suffixes.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 51
as ./int/tests/suffixes.s -o ./int/tests/suffixes.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 58
gcc -static ./int/tests/suffixes.o -o ./bin/tests/suffixes
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 20
./bin/tests/suffixes
:i returncode 69
:b stdout 0

:b stderr 0

