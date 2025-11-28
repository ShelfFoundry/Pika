#include "base.h"
#include "heap.h"
#include "assert.h"
#include "ecs_registry.h"
#include <stdalign.h>

struct transform {
    i32 x;
    i32 y;
    i32 z;
    u32 w;
    u32 h;
    u32 d;
    i32 rx;
    i32 ry;
    i32 rz;
};

struct transform_pool {
    u32 capacity;
    u8 *slots;
    u8 *gen;
};

void transform_pool_init(struct transform_pool *t, u32 initial_capacity)
{
    t->capacity = initial_capacity;
    size_t slot_bytes = sizeof(struct transform) * initial_capacity;
    u8* slots_arr = heap_alloc(slot_bytes, alignof(struct transform));
    if (!slots_arr) return out_of_memory();
    t->slots = slots_arr;

    struct transform *slots = (struct transform*)t->slots;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        slots[i].x = 0;
        slots[i].y = 0;
        slots[i].z = 0;
        slots[i].w = FOOT;
        slots[i].h = FOOT;
        slots[i].d = FOOT;
        slots[i].rx = 0;
        slots[i].ry = 0;
        slots[i].rz = 0;
    }

    size_t gen_bytes = sizeof(u16) * initial_capacity;
    u8* gen_arr = heap_alloc(gen_bytes, alignof(u16));
    if (!gen_arr) return out_of_memory();
    t->gen = gen_arr;

    u16 *gen = (u16*)t->gen;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        gen[i] = 1;
    }
}

struct transform_pool* transform_pool_create(u32 initial_capacity)
{
    struct transform_pool* t = (struct transform_pool*)heap_alloc(sizeof *t, alignof(struct transform_pool));
    transform_pool_init(t, initial_capacity);
    return t;
}

void destroy_transform(struct transform_pool *t, u32 handle, u16 new_gen)
{
    u32 idx = handle_idx(handle);
    assert(idx < t->capacity);
    u16 gen = handle_gen(handle);

    u16 *gen_arr = (u16*)t->gen;
    if (gen_arr[idx] != gen) return; // NOTE: stale handle noop

    struct transform *slots = (struct transform*)t->slots;
    slots[idx].x = 0;
    slots[idx].y = 0;
    slots[idx].z = 0;
    slots[idx].w = FOOT;
    slots[idx].h = FOOT;
    slots[idx].d = FOOT;
    slots[idx].rx = 0;
    slots[idx].ry = 0;
    slots[idx].rz = 0;

    gen_arr[idx] = new_gen;
}

struct transform* get_transform(struct transform_pool *t, u32 handle)
{
    u32 idx = handle_idx(handle);
    assert(idx < t->capacity);
    u16 gen = handle_gen(handle);

    u16 *gen_arr = (u16*)t->gen;
    if (gen_arr[idx] != gen) return NULL; // NOTE: stale handle noop

    struct transform *slots = (struct transform*)t->slots;
    return &slots[idx];
}
