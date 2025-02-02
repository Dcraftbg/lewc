:i count 4
:b shell 62
./bin/lewc examples/local_vars.lew -o ./int/tests/local_vars.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 55
as ./int/tests/local_vars.s -o ./int/tests/local_vars.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 62
gcc -static ./int/tests/local_vars.o -o ./bin/tests/local_vars
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 22
./bin/tests/local_vars
:i returncode 0
:b stdout 12
ret_69 => 69
:b stderr 0

