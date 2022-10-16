#!/bin/bash

# make clean && make
prog=$1

# gcc ./test/${prog}.c -o ./test/${prog}
# echo "sysy:"
# ./test/${prog}; echo $?

./build/compiler -koopa ./test/${prog}.c -o ./test/${prog}.koopa
koopac ./test/${prog}.koopa | llc --filetype=obj -o ./test/${prog}.o
clang ./test/${prog}.o -L$CDE_LIBRARY_PATH/native -lsysy -o ./test/${prog}
echo "koopa:"
./test/${prog}; echo $?

./build/compiler -riscv ./test/${prog}.c -o ./test/${prog}.S
clang ./test/${prog}.S -c -o ./test/${prog}.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld ./test/${prog}.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o ./test/${prog}
echo "riscv:"
qemu-riscv32-static ./test/${prog}; echo $?
