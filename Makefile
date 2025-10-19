SRC := src
INC := include
OBJ := build
CFLAGS := -std=c17 -O3 -Wall -Wextra -Wswitch-enum -fno-builtin -I$(INC) -I$(SRC) -DPLATFORM_WEB -MMD -MP
WASMFLAGS := --target=wasm32 -nostdlib
LDFLAGS := -Wl,--no-entry -Wl,--allow-undefined

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(OBJ)/app.wasm

$(OBJ)/app.wasm: $(OBJS)
	clang $(WASMFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	clang $(CFLAGS) $(WASMFLAGS) -c $< -o $@

$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm -rf $(OBJ)

-include $(DEPS)

.PHONY: compdb
compdb:
	rm -f compile_commands.json
	bear -- make -B
