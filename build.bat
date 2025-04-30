@echo off

if "%1" == "" clang -O2 -o flos.exe ./src/linuxMain.c -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS
if "%1" == "test" clang -g -o test.exe ./tests/main.c -I./src -I./ -DWINDOWS -D_CRT_SECURE_NO_WARNINGS && .\test.exe --enable-mixed-units
