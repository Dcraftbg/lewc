:i count 1
:b shell 52
./bin/lewc examples/nested.lew -o int/tests/nested.s
:i returncode 1
:b stdout 0

:b stderr 67
ERROR:examples/nested.lew:2:8: Unexpected token in expression: ':'

