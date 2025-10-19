#include "base.h"
#include "heap.h"
#include "assert.h"

// TODO: remove
static f32 bg_color_flip_timer = 5.0;
static u32 bg_color = 0xFFFF0000;

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
static struct framebuffer display = {0};

struct frame {
    uintptr_t ptr;
    u32 stride;
    u32 width;
    u32 height;
    u32 bpp;
    u32 version;
};
static struct frame frame = {
    .bpp = 4,
    .version = 0,
};
WASM_EXPORT(get_frame_addr)
u32 get_frame_addr()
{
    return (u32)(uintptr_t)&frame;
}

static inline u8* pixel_ptr(const struct framebuffer* d, size_t x, size_t y)
{
    return d->buffer + y * d->stride + x * BYTES_PER_PIXEL;
}

WASM_EXPORT(engine_init)
void engine_init()
{
    heap_init();
    display.align = 64;
}

WASM_EXPORT(set_display_size)
void set_display_size(u32 width, u32 height)
{
    assert(width > 0 && height > 0);

    display.needs_repaint = 1;
    display.width_px = width;
    display.height_px = height;
    frame.version = 0;
    frame.width = display.width_px;
    frame.height = display.height_px;

    // NOTE: early return if the new canvas is smaller than the largest possible. No allocation needed.
    if (width <= display.max_width_px && height <= display.max_height_px)
    {
        return;
    }

    console_log();
    display.max_width_px = width;
    display.max_height_px = height;
    assert(display.width_px <= display.max_width_px && display.height_px <= display.max_height_px);

    u8* base = alloc_display_buffer(display.max_width_px, display.max_height_px, display.align, &display.stride);
    if (!base) return out_of_memory();
    display.buffer = base;
    assert(display.stride % BYTES_PER_PIXEL == 0);
    assert(display.stride >= (size_t)display.max_width_px * BYTES_PER_PIXEL);

    size_t back_stride;
    u8* back_base = alloc_display_buffer(display.max_width_px, display.max_height_px, display.align, &back_stride);
    if (!back_base) return out_of_memory();
    display.back_buffer = back_base;
    assert(back_stride == display.stride);

    frame.stride = display.stride;
    frame.ptr = (uintptr_t)display.buffer;
}

WASM_EXPORT(render)
void render()
{
    if (display.dirty || display.needs_repaint)
    {
        // NOTE: backfill canvas with grey
        for (u32 y = 0; y < display.height_px; y++) {
            u32 *row = (u32 *)(display.back_buffer + y * display.stride);
            for (u32 x = 0; x < display.width_px + display.stride; x++) {
                row[x] = bg_color;
            }
        }

        u8* p = display.buffer;
        display.buffer = display.back_buffer;
        display.back_buffer = p;
        frame.ptr = (uintptr_t)display.buffer;
        frame.version++;
        display.needs_repaint = 0;
    }
}

WASM_EXPORT(update)
void update(f32 dt)
{
    display.dirty = 0;

    bg_color_flip_timer -= dt;

    if (bg_color_flip_timer <= 0)
    {
        if (bg_color == 0xFFFF0000)
        {
            bg_color = 0xFF00FF00;
        }
        else if (bg_color == 0xFF00FF00)
        {
            bg_color = 0xFF0000FF;
        }
        else if (bg_color == 0xFF0000FF)
        {
            bg_color = 0xFFFF0000;
        }
        display.dirty = 1;
        bg_color_flip_timer = 5.0;
    }
}

#ifdef PLATFORM_NATIVE
int main(void)
{
    game_init();
    return 0;
}
#endif
