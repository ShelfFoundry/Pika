#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include "base.h"

void heap_init(void);

u8* heap_alloc(size_t bytes, size_t align);

u8* alloc_display_buffer(u32 width_px, u32 height_px, size_t align_bytes, size_t* out_stride_bytes);

#endif
