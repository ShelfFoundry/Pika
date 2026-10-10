## Prereqs
- [Clang/LLVM (wasm32)](https://llvm.org)
- [GNU Make](https://www.gnu.org/software/make/)
- [Bear (compile_commands.json)](https://github.com/rizsotto/Bear) (for LSP, optional)
- [Python 3](https://www.python.org/) (for local static server, optional)

**Install commands**:

- macOS: `brew install llvm bear`
- Ubuntu/Debian: `sudo apt-get install clang make bear python3`
- Arch (btw): `sudo pacman -S clang make bear python3`

## Layout

```bash
/include/           # public headers (e.g., base.h, heap.h)
/src/               # .c files (e.g., main.c, heap.c)
/build/             # artifacts (generated)
Makefile
index.html
main.js
main.css
renderer.js         # WebGL2 render pipeline
shader-program.js   # WebGL2 shader program utility
frame-view.js       # DataView for framebuffer
```

## Build (WASM)

```bash
make                # -> build/app.wasm
make clean
```

**Notes**:

- Target: `wasm32 -nostdlib` with `--allow-undefined` for host imports.
- If JS must call a C function annotated with the definition:

```c
WASM_EXPORT(my_func)
void my_func(void) { /* ... */ }
```

## LSP (clangd)

Run when you add/remove `.c` files or change flags/includes.

```bash
make compdb     # full rebuild with Bear
```

## Serve Locally (static files)

Serve repo root:

```bash
python3 -m http.server 8000 --bind 127.0.0.1
```
