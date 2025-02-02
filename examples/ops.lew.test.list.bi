:i count 4
:b shell 48
./bin/lewc examples/ops.lew -o ./int/tests/ops.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 41
as ./int/tests/ops.s -o ./int/tests/ops.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 65
gcc -static ./int/tests/ops.o ./examples/ops.c -o ./bin/tests/ops
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 15
./bin/tests/ops
:i returncode 0
:b stdout 0

:b stderr 517
[Test 0] 4 + 5 => 9 (add_lew)...OK
[Test 1] 4 - 5 => -1 (sub_lew)...OK
[Test 2] 4 * 5 => 20 (mul_lew)...OK
[Test 3] 4 / 5 => 0 (div_lew)...OK
[Test 4] 4 % 5 => 4 (mod_lew)...OK
[Test 5] 4 & 5 => 4 (and_lew)...OK
[Test 6] 4 | 5 => 5 (or_lew)...OK
[Test 7] 4 ^ 5 => 1 (xor_lew)...OK
[Test 8] 4 << 5 => 128 (shl_lew)...OK
[Test 9] 4 >> 5 => 0 (shr_lew)...OK
[Test 10] 4 < 5 => 1 (bool_lt_lew)...OK
[Test 11] 4 <= 5 => 1 (bool_le_lew)...OK
[Test 12] 4 > 5 => 0 (bool_gt_lew)...OK
[Test 13] 4 >= 5 => 0 (bool_ge_lew)...OK

