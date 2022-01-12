main.exe: src/main.cpp mongoose.o res/res.o
	g++ src/main.cpp mongoose.o res/res.o -luser32 -lgdi32 -lshell32 -lshlwapi -lws2_32 -static-libgcc -static-libstdc++ -Os -s -o main.exe -mwindows

mongoose.o: src/mongoose.c
	g++ -c src/mongoose.c

res/res.o: res/res.rc res/main.exe.manifest res/icon_white.ico
	windres res/res.rc res/res.o

res/icon_white.ico: res/icon.svg
	magick convert -background none res/icon.svg -define icon:auto-resize -fuzz 60% -fill green -opaque white res/icon_green.ico
	magick convert -background none res/icon.svg -define icon:auto-resize -fuzz 60% -fill red -opaque white res/icon_red.ico
	magick convert -background none res/icon.svg -define icon:auto-resize res/icon_white.ico