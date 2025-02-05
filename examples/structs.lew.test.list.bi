:i count 4
:b shell 56
./bin/lewc examples/structs.lew -o ./int/tests/structs.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 49
as ./int/tests/structs.s -o ./int/tests/structs.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 75
gcc -static ./int/tests/structs.o examples/structs.c -o ./bin/tests/structs
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 19
./bin/tests/structs
:i returncode 0
:b stdout 188
Values:
.a = 34
.b = 35
take_ptr_and_ret_value:
.a = 34
.b = 35
take_value_and_ret_value:
.a = 34
.b = 35
get_a = 34
get_b = 35
set_vals_test_copy:
.a = 34
.b = 35
set_vals:
.a = 4
.b = 5

:b stderr 0

