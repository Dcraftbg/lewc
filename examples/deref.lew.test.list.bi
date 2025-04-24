:i count 3
:b shell 52
./bin/lewc examples/deref.lew -o ./int/tests/deref.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/deref.o -o ./bin/tests/deref
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/deref
:i returncode 0
:b stdout 1
H
:b stderr 0

