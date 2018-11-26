STB_INCLUDE_PATH = /home/hex/libraries/stb

CFLAGS = -std=c++17 -I/usr/include/freetype2 -I$(STB_INCLUDE_PATH)

LDFLAGS = `pkg-config --static --libs glfw3` -lglut -lGL -lfreetype

OpenGLTest: main.cpp
		g++ $(CFLAGS) -o Test main.cpp glad.c $(LDFLAGS)

.PHONY: test clean

test: Test
		./Test

clean:
		rm -f Test
