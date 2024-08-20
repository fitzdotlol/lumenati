SOURCE_FILES=src/file.c src/gtx.c src/nut.c src/lumen.c src/texlist.c src/main.c

all:
	mkdir -p bin
	cc -g -ggdb -o bin/lumenati $(SOURCE_FILES) -lraylib -lm