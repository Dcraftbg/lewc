:i count 3
:b shell 52
./bin/lewc examples/hello.lew -o ./int/tests/hello.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/hello.o -o ./bin/tests/hello
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/hello
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

