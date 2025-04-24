#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Please specify name"
    echo "Usage: $0 <path without .lew>"
    exit 1
fi

test_list="./bin/lewc examples/$1.lew -o ./int/tests/$1.o
gcc -static ./int/tests/$1.o -o ./bin/tests/$1
./bin/tests/$1"
echo "$test_list" > "$1.lew.test.list"
