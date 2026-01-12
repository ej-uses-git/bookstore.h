all: build
	./build

.PHONY: all

build: build.c
	[[ -x build ]] || gcc -o build build.c
