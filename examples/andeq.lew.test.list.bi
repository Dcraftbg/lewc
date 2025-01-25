:i count 4
:b shell 52
./bin/lewc examples/andeq.lew -o ./int/tests/andeq.s
:i returncode 1
:b stdout 0

:b stderr 66
ERROR:examples/andeq.lew:2:6: Unexpected token in expression: '='

:b shell 45
as ./int/tests/andeq.s -o ./int/tests/andeq.o
:i returncode 1
:b stdout 0

:b stderr 97
Assembler messages:
Error: can't open ./int/tests/andeq.s for reading: No such file or directory

:b shell 52
gcc -static ./int/tests/andeq.o -o ./bin/tests/andeq
:i returncode 1
:b stdout 0

:b stderr 115
/usr/bin/ld: cannot find ./int/tests/andeq.o: No such file or directory
collect2: error: ld returned 1 exit status

:b shell 17
./bin/tests/andeq
:i returncode 127
:b stdout 0

:b stderr 36
sh: 1: ./bin/tests/andeq: not found

