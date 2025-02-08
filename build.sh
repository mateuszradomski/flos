#!/bin/bash
if [ "$1" == "" ]; then
  clang -O2 -o solfmt ./src/linuxMain.c -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS
fi
if [ "$1" == "test" ]; then
  clang -g -o test ./tests/main.c -I./src -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS && ./test --enable-mixed-units
fi
