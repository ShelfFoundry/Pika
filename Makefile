all: wasm

wasm: ./src/main.c
	clang -Wall -Wextra -Wswitch-enum -O3 -fno-builtin --target=wasm32 --no-standard-libraries \
		-Wl,--no-entry -Wl,--allow-undefined \
		-o main.wasm ./src/main.c -DPLATFORM_WEB

native: ./src/main.c
	clang -o main ./src/main.c -lm -DPLATFORM_NATIVE
