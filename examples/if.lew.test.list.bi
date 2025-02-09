:i count 4
:b shell 46
./bin/lewc examples/if.lew -o ./int/tests/if.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 39
as ./int/tests/if.s -o ./int/tests/if.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 46
gcc -static ./int/tests/if.o -o ./bin/tests/if
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 14
./bin/tests/if
:i returncode 0
:b stdout 37
a> Nice
a> Even nicer
a> Not nice :(

:b stderr 0

