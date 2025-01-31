:i count 4
:b shell 60
./bin/lewc examples/constants.lew -o ./int/tests/constants.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 53
as ./int/tests/constants.s -o ./int/tests/constants.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 60
gcc -static ./int/tests/constants.o -o ./bin/tests/constants
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 21
./bin/tests/constants
:i returncode 0
:b stdout 7
Nice 69
:b stderr 0

