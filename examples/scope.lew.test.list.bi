:i count 3
:b shell 52
./bin/lewc examples/scope.lew -o ./int/tests/scope.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/scope.o -o ./bin/tests/scope
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/scope
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

