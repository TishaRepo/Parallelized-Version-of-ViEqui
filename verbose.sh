#!/bin/bash

clang -c -emit-llvm -S -o executable_file.ll $1
./src/nidhugg --sc --view --check-optimality --print-progress executable_file.ll
rm executable_file.ll
