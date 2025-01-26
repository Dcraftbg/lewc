:i count 4
:b shell 58
./bin/lewc examples/find_str.lew -o ./int/tests/find_str.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 51
as ./int/tests/find_str.s -o ./int/tests/find_str.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 80
gcc -static ./examples/find_str.c ./int/tests/find_str.o -o ./bin/tests/find_str
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 20
./bin/tests/find_str
:i returncode 0
:b stdout 36
str="ooBar"
find_unique(o) => "Bar"

:b stderr 0

