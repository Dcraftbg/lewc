:i count 1
:b shell 64
./bin/lewc examples/syntactical.lew -o ./int/tests/syntactical.o
:i returncode 1
:b stdout 0

:b stderr 71
ERROR examples/syntactical.lew:2:4: Unknown variable or function `foo`

