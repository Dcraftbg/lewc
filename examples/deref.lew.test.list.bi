:i count 4
:b shell 52
./bin/lewc examples/deref.lew -o ./int/tests/deref.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 45
as ./int/tests/deref.s -o ./int/tests/deref.o
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

