#!/bin/bash
cd ..
if [ $# -ne 1 ]; then
    echo "Expected 1 parameter, seed."
    exit
fi
./build/isl_tester -m SET_META -s $1
cd ./out
./compile.sh
timeout 120 ./test
echo $?
cd ..
