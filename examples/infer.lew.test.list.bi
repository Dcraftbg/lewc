:i count 4
:b shell 52
./bin/lewc examples/infer.lew -o ./int/tests/infer.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 45
as ./int/tests/infer.s -o ./int/tests/infer.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/infer.o -o ./bin/tests/infer
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/infer
:i returncode 69
:b stdout 0

:b stderr 0

