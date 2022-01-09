
REM cl src/mongoose.c -c

cl /EHsc src/main.cpp mongoose.obj /link user32.lib gdi32.lib shell32.lib Shlwapi.lib ws2_32.lib
mt -manifest .\main.exe.manifest -outputresource:main.exe;1