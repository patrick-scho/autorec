@echo off

REM cl src/mongoose.c -c
REM rc res/res.rc

cl /EHsc src/main.cpp src/mongoose.c /Fo"./out"/ /link user32.lib gdi32.lib shell32.lib Shlwapi.lib ws2_32.lib res/res.res /out:out/main.exe
REM mt -manifest res/main.exe.manifest -outputresource:main.exe;1