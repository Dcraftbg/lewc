:i count 3
:b shell 56
./bin/lewc examples/size_of.lew -o ./int/tests/size_of.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 56
gcc -static ./int/tests/size_of.o -o ./bin/tests/size_of
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 19
./bin/tests/size_of
:i returncode 0
:b stdout 19
size_of(Foo) => 16

:b stderr 0

