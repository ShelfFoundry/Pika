#include "base.h"
#include "heap.h"
#include "assert.h"
#include "ecs_registry.h"
#include <stddef.h>
#include <stdint.h>

static u32 bg_color = 0xFF181818;

struct framebuffer {
    u8 *buffer;
    u8 *back_buffer;
    size_t stride;
    u32 width_px;
    u32 height_px;
    u32 max_width_px;
    u32 max_height_px;
    size_t align;
    b32 dirty;
    b32 needs_repaint;
};

struct frame {
    uintptr_t ptr;
    u32 stride;
    u32 width;
    u32 height;
    u32 bpp;
    u32 version;
};

static struct {
    struct entity_registry *registry;
    struct framebuffer display;
    struct frame frame;
} engine = {0};

WASM_EXPORT(get_frame_addr)
u32 get_frame_addr()
{
    return (u32)(uintptr_t)&engine.frame;
}

static inline u8* pixel_ptr(const struct framebuffer* d, size_t x, size_t y)
{
    return d->buffer + y * d->stride + x * BYTES_PER_PIXEL;
}

WASM_EXPORT(engine_init)
void engine_init()
{
    heap_init();
    engine.registry = registry_create(1024);
    engine.display.align = 64;
    engine.frame.bpp = 4;
    engine.frame.version = 0;
}

WASM_EXPORT(set_display_size)
void set_display_size(u32 width, u32 height)
{
    assert(width > 0 && height > 0);

    engine.display.needs_repaint = 1;
    engine.display.width_px = width;
    engine.display.height_px = height;
    engine.frame.version = 0;
    engine.frame.width = engine.display.width_px;
    engine.frame.height = engine.display.height_px;

    // NOTE: early return if the new canvas is smaller than the largest possible. No allocation needed.
    if (width <= engine.display.max_width_px && height <= engine.display.max_height_px)
    {
        return;
    }

    engine.display.max_width_px = width;
    engine.display.max_height_px = height;
    assert(engine.display.width_px <= engine.display.max_width_px && engine.display.height_px <= engine.display.max_height_px);

    u8* base = alloc_display_buffer(engine.display.max_width_px, engine.display.max_height_px, engine.display.align, &engine.display.stride);
    if (!base) return out_of_memory();
    engine.display.buffer = base;
    assert(engine.display.stride % BYTES_PER_PIXEL == 0);
    assert(engine.display.stride >= (size_t)engine.display.max_width_px * BYTES_PER_PIXEL);

    size_t back_stride;
    u8* back_base = alloc_display_buffer(engine.display.max_width_px, engine.display.max_height_px, engine.display.align, &back_stride);
    if (!back_base) return out_of_memory();
    engine.display.back_buffer = back_base;
    assert(back_stride == engine.display.stride);

    engine.frame.stride = engine.display.stride;
    engine.frame.ptr = (uintptr_t)engine.display.buffer;
}

WASM_EXPORT(render)
void render()
{
    if (engine.display.dirty || engine.display.needs_repaint)
    {
        // NOTE: backfill canvas with grey
        for (u32 y = 0; y < engine.display.height_px; y++) {
            u32 *row = (u32*)(engine.display.back_buffer + y * engine.display.stride);
            for (u32 x = 0; x < engine.display.width_px + engine.display.stride; x++) {
                row[x] = bg_color;
            }
        }

        u8* p = engine.display.buffer;
        engine.display.buffer = engine.display.back_buffer;
        engine.display.back_buffer = p;
        engine.frame.ptr = (uintptr_t)engine.display.buffer;
        engine.frame.version++;
        engine.display.needs_repaint = 0;
    }
}

WASM_EXPORT(update)
void update(f32 dt)
{
    engine.display.dirty = 0;
}

#ifdef PLATFORM_NATIVE
int main(void)
{
    game_init();
    return 0;
}
#endif
