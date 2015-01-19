@echo off
gcc -g -o ttt.exe ttt.c -lgdi32 -lopengl32 -lglu32 -mwindows
pause
ttt.exe