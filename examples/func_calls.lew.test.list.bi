:i count 3
:b shell 62
./bin/lewc examples/func_calls.lew -o ./int/tests/func_calls.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 62
gcc -static ./int/tests/func_calls.o -o ./bin/tests/func_calls
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 22
./bin/tests/func_calls
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

