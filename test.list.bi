:i count 21
:b shell 48
./bin/lewc examples/sum.lew -o ./int/tests/sum.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 41
as ./int/tests/sum.s -o ./int/tests/sum.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 57
gcc ./examples/sum.c ./int/tests/sum.o -o ./bin/tests/sum
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 15
./bin/tests/sum
:i returncode 0
:b stdout 14
sum(4,5) => 9

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
./bin/lewc examples/hello.lew -o ./int/tests/hello.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 45
as ./int/tests/hello.s -o ./int/tests/hello.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 52
gcc -static ./int/tests/hello.o -o ./bin/tests/hello
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 17
./bin/tests/hello
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 62
./bin/lewc examples/func_calls.lew -o ./int/tests/func_calls.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 55
as ./int/tests/func_calls.s -o ./int/tests/func_calls.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 62
gcc -static ./int/tests/func_calls.o -o ./bin/tests/func_calls
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 22
./bin/tests/func_calls
:i returncode 0
:b stdout 13
Hello World!

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 66
./bin/lewc examples/int_literals.lew -o ./int/tests/int_literals.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 59
as ./int/tests/int_literals.s -o ./int/tests/int_literals.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 84
gcc ./examples/int_literals.c ./int/tests/int_literals.o -o ./bin/tests/int_literals
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 24
./bin/tests/int_literals
:i returncode 0
:b stdout 15
get_five() = 5

:b stderr 0

:b shell 0

:i returncode 0
:b stdout 0

:b stderr 0

:b shell 64
./bin/lewc examples/syntactical.lew -o ./int/tests/syntactical.s
:i returncode 1
:b stdout 0

:b stderr 35
Unknown variable or function `foo`

