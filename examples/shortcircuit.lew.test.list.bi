:i count 4
:b shell 66
./bin/lewc examples/shortcircuit.lew -o ./int/tests/shortcircuit.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 59
as ./int/tests/shortcircuit.s -o ./int/tests/shortcircuit.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 66
gcc -static ./int/tests/shortcircuit.o -o ./bin/tests/shortcircuit
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 24
./bin/tests/shortcircuit
:i returncode 0
:b stdout 10
Called a!

:b stderr 0

