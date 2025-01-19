:i count 1
:b shell 64
./bin/lewc examples/syntactical.lew -o ./int/tests/syntactical.s
:i returncode 1
:b stdout 0

:b stderr 35
Unknown variable or function `foo`

