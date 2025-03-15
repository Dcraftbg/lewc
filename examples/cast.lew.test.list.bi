:i count 1
:b shell 50
./bin/lewc examples/cast.lew -o ./int/tests/cast.s
:i returncode 0
:b stdout 0

:b stderr 0

