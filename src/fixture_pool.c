#include "base.h"
#include "heap.h"
#include "assert.h"
#include "ecs_registry.h"
#include <stdalign.h>

struct fixture {
    u32 foo;
    i32 bar;
    flags32 baz;
};

struct fixture_pool {
    u32 capacity;
    u8 *slots;
    u8 *gens;
    u32 free_head;
    u8 *next_free;
};

void fixture_pool_init(struct fixture_pool *p, u32 initial_capacity)
{
    p->capacity = initial_capacity;
    size_t slot_bytes = sizeof(struct fixture) * initial_capacity;
    u8* slots_arr = heap_alloc(slot_bytes, alignof(struct fixture));
    if (!slots_arr) return out_of_memory();
    p->slots = slots_arr;

    size_t next_bytes = sizeof(u32) * initial_capacity;
    u8* next_arr = heap_alloc(next_bytes, alignof(u32));
    if (!next_arr) return out_of_memory();
    p->next_free = next_arr;
    u32 *next = (u32*)p->next_free;
    for (u32 i = 0; i < initial_capacity - 1; i++)
    {
        next[i] = i + 1;
    }
    next[initial_capacity - 1] = INVALID_U32;
    p->free_head = 0;

    size_t gen_bytes = sizeof(u16) * initial_capacity;
    u8* gen_arr = heap_alloc(gen_bytes, alignof(u16));
    if (!gen_arr) return out_of_memory();
    p->gens = gen_arr;
    u16 *gen = (u16*)p->gens;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        gen[i] = 1;
    }
}

struct fixture_pool* fixture_pool_create(u32 initial_capacity)
{
    struct fixture_pool* p = (struct fixture_pool*)heap_alloc(sizeof *p, alignof(struct fixture_pool));
    fixture_pool_init(p, initial_capacity);
    return p;
}

