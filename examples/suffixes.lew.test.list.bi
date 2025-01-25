:i count 1
:b shell 58
./bin/lewc examples/suffixes.lew -o ./int/tests/suffixes.s
:i returncode 1
:b stdout 0

:b stderr 46
Return type mismatch. Expected i32 but got u8

