set MODE=Release
echo F|xcopy /y ..\background\ConsoleApplication1\x64\%MODE%\ConsoleApplication1.exe binary.exe
echo F|xcopy /y ..\background\ConsoleApplication1\ConsoleApplication1\sqlite3.dll sqlite3.dll