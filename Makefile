all: build run

build:
	gcc -O3 -Wall -g -std=c99 ./src/main.c -lSDL2 -o tinydraw -lSDL2_image

run:
	./tinydraw

clean:
	rm -rf tinydraw tinydraw.dSYM
