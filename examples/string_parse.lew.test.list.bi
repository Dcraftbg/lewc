:i count 3
:b shell 66
./bin/lewc examples/string_parse.lew -o ./int/tests/string_parse.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 66
gcc -static ./int/tests/string_parse.o -o ./bin/tests/string_parse
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 24
./bin/tests/string_parse
:i returncode 0
:b stdout 10
lo World!

:b stderr 0

