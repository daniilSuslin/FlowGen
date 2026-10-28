@echo off
g++ -std=c++17 -O2 -mwindows -static src\flowgen.cpp -o FlowGen.exe -lcomctl32 -lcomdlg32 -lgdi32 -luser32 -lshlwapi
if errorlevel 1 (echo Build failed) else (echo Done: flowgen.exe)
