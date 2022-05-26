#!/bin/bash

clang -c -emit-llvm -S -o executable_file.ll $1
time timeout 1800s ./src/nidhugg --sc --view executable_file.ll
rm executable_file.ll