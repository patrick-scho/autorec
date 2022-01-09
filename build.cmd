
REM cl src/mongoose.c -c
REM rc res/res.rc

cl /EHsc src/main.cpp mongoose.obj /link user32.lib gdi32.lib shell32.lib Shlwapi.lib ws2_32.lib res/res.res
mt -manifest .\main.exe.manifest -outputresource:main.exe;1