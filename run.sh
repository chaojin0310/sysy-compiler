#!/bin/bash

# make clean && make
prog=$1

./build/compiler -koopa ./test/${prog}.c -o ./test/${prog}.koopa

./build/compiler -riscv ./test/${prog}.c -o ./test/${prog}.S
