#include "base.h"
#include "heap.h"
#include "assert.h"
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

#define INVALID_U32 0xFFFFFFFF
#define IDX_BITS 20
#define GEN_BITS 12
#define IDX_MASK (1u<<IDX_BITS)-1
#define GEN_MASK (1u<<GEN_BITS)-1
#define GEN_SHIFT IDX_BITS
#define NULL_HANDLE 0u

struct entity {
    u16 gen;
    flags32 mask;
};

struct entity_registry {
    u32 capacity;
    u32 free_head;
    u8 *next_free;
    u8 *slots;
};

static inline handle_t handle_pack(u32 idx, u16 gen)
{
    // NOTE: (idx & IDX_MASK): keep only low 20 bits
    // ((u32)gen & GEN_MASK) << GEN_SHIFT: place gen in upper 12 bits
    assert(idx <= IDX_MASK);
    assert((gen & ~GEN_MASK) == 0);
    return (idx & IDX_MASK) | (((u32)gen & GEN_MASK) << GEN_SHIFT);
}

static inline u32 handle_idx(handle_t handle)
{
    return handle & IDX_MASK;
}

static inline u16 handle_gen(handle_t handle)
{
    return (u16)(handle >> GEN_SHIFT) & GEN_MASK;
}

void registry_init(struct entity_registry *r, u32 initial_capacity)
{
    r->capacity = initial_capacity;
    size_t slot_bytes = sizeof(struct entity) * initial_capacity;
    u8* slots_arr = heap_alloc(slot_bytes, alignof(struct entity));
    if (!slots_arr) return out_of_memory();
    r->slots = slots_arr;
    size_t next_bytes = sizeof(u32) * initial_capacity;
    u8* next_arr = heap_alloc(next_bytes, alignof(u32));
    if (!next_arr) return out_of_memory();
    r->next_free = next_arr;

    u32 *next = (u32*)r->next_free;
    for (u32 i = 0; i < initial_capacity - 1; i++)
    {
        next[i] = i + 1;
    }
    next[initial_capacity - 1] = INVALID_U32;
    r->free_head = 0;

    struct entity *slots = (struct entity*)r->slots;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        slots[i].gen = 1;
        slots[i].mask = 0;
    }

    console_log();
}

handle_t registry_create_entity(struct entity_registry *r)
{
    // TODO: refactor to automatically grow when exhausted
    assert(r->free_head != INVALID_U32 && "Entity pool exhausted");
    struct entity *slots = (struct entity*)r->slots;
    u32 idx = r->free_head;
    assert(slots[idx].gen != 0);
    assert(slots[idx].mask == 0);
    u32 *next = (u32*)r->next_free;
    r->free_head = next[idx];
    handle_t handle = handle_pack(idx, slots[idx].gen);
    assert(handle != NULL_HANDLE);
    return handle;
}

void registry_destroy_entity(struct entity_registry *r, u32 handle)
{
    u32 idx = handle_idx(handle);
    assert(idx < r->capacity);
    u16 gen = handle_gen(handle);

    struct entity *slots = (struct entity*)r->slots;
    if (slots[idx].gen != gen) return; // NOTE: stale handle noop

    //flags32 m = slots[idx].mask;
    // TODO: destroy components based on mask before reset
    slots[idx].mask = 0;

    slots[idx].gen++;
    if (slots[idx].gen == 0) return; // NOTE: tombstoned

    u32 *next = (u32*)r->next_free;
    next[idx] = r->free_head;
    r->free_head = idx;
}

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
    struct entity_registry registry;
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
    registry_init(&engine.registry, 1024);
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
