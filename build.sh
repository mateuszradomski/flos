#!/bin/bash
if [ "$1" == "" ]; then
  clang -O2 -o flos ./src/linuxMain.c -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS -pthread
fi
if [ "$1" == "test" ]; then
  clang -g -o test ./tests/main.c ./tests/munit.c -I./src -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS -DMUNIT_TEST_NAME_LEN=65 && ./test
fi
