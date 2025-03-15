:i count 4
:b shell 52
./bin/lewc examples/defer.lew -o ./int/tests/defer.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 45
as ./int/tests/defer.s -o ./int/tests/defer.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/defer.o -o ./bin/tests/defer
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/defer
:i returncode 0
:b stdout 24
We did this
Using defer

:b stderr 0

