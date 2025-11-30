#include "base.h"
#include "heap.h"
#include "assert.h"
#include "ecs_registry.h"
#include <stdalign.h>
#include <stdint.h>

struct fixture {
    u32 foo;
    i32 bar;
    flags32 baz;
};

struct fixture_pool {
    u32 capacity;
    u32 count;
    u8 *slots;
    u8 *gens;
    u8 *dense_to_entity;
    u8 *sparse_from_entity;
};

void fixture_pool_init(struct fixture_pool *p, u32 initial_capacity)
{
    p->capacity = initial_capacity;
    size_t slot_bytes = sizeof(struct fixture) * initial_capacity;
    u8* slots_arr = heap_alloc(slot_bytes, alignof(struct fixture));
    if (!slots_arr) return out_of_memory();
    p->slots = slots_arr;

    size_t dense_bytes = sizeof(u32) * initial_capacity;
    u8* dense_arr = heap_alloc(dense_bytes, alignof(u32));
    if (!dense_arr) return out_of_memory();
    p->dense_to_entity = dense_arr;
    p->count = 0;

    size_t gen_bytes = sizeof(u16) * initial_capacity;
    u8* gen_arr = heap_alloc(gen_bytes, alignof(u16));
    if (!gen_arr) return out_of_memory();
    p->gens = gen_arr;
    u16 *gen = (u16*)p->gens;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        gen[i] = 1;
    }

    size_t sparse_bytes = sizeof(u16) * UINT16_MAX;
    u8* sparse_arr = heap_alloc(sparse_bytes, alignof(u16));
    if (!sparse_arr) return out_of_memory();
    p->sparse_from_entity = sparse_arr;
    u16 *sparse = (u16*)p->sparse_from_entity;
    for (u32 i = 0; i < UINT16_MAX; i++)
    {
        sparse[i] = 0;
    }
}

struct fixture_pool* fixture_pool_create(u32 initial_capacity)
{
    struct fixture_pool* p = (struct fixture_pool*)heap_alloc(sizeof *p, alignof(struct fixture_pool));
    fixture_pool_init(p, initial_capacity);
    return p;
}

