@echo off

gcc -I "./inc/" "./src/*" -o neo_win32.exe -static -lWS2_32 -D C_WIN32

neo_win32

if not errorlevel 0 echo Exit code: %errorlevel%
