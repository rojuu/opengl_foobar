all: main

SOURCES=src/main.cpp

main: $(SOURCES)
	mkdir -p bin
	g++ $(SOURCES) -o bin/opengl_foobar -lassimp -lglfw -lGLEW -lGL
