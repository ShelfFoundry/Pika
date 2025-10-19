#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <stdint.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef int16_t i16;
typedef uint16_t u16;
typedef uint32_t flags32;
typedef float f32;
typedef uint8_t u8;
typedef int8_t i8;
typedef uint32_t b32;
typedef uint32_t handle_t;

#ifdef PLATFORM_WEB
  #define WASM_EXPORT(name) __attribute__((export_name(#name))) __attribute__((used))
#else
  #define WASM_EXPORT(name)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

extern void out_of_memory(void);
extern void console_log(void);

enum { BYTES_PER_PIXEL = 4 }; // RGBA
enum { PAGE_SIZE = 65536 };

#endif
