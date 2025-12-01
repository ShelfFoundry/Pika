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
    struct fixture *slots = (struct fixture*)p->slots;
    for (u32 i = 0; i < initial_capacity; i++)
    {
        // TODO: replace with actual fields
        slots[i].foo = 0;
        slots[i].bar = 0;
        slots[i].baz = 0;
    }

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

    size_t sparse_bytes = sizeof(u16) * MAX_ENTITIES_FOR_SPARSE;
    u8* sparse_arr = heap_alloc(sparse_bytes, alignof(u16));
    if (!sparse_arr) return out_of_memory();
    p->sparse_from_entity = sparse_arr;
    u16 *sparse = (u16*)p->sparse_from_entity;
    for (u32 i = 0; i < MAX_ENTITIES_FOR_SPARSE; i++)
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

handle_t fixture_alloc(struct fixture_pool *p, handle_t handle)
{
    u32 entity_idx = handle_idx(handle);
    assert(entity_idx < MAX_ENTITIES_FOR_SPARSE);
    u32 dense_idx = p->count;
    assert(dense_idx < p->capacity); // TODO: handle growing array
    p->count++;
    u32 *dense = (u32*)p->dense_to_entity;
    dense[dense_idx] = entity_idx;
    u16 *sparse = (u16*)p->sparse_from_entity;
    assert(sparse[entity_idx] == 0);
    sparse[entity_idx] = dense_idx + 1;

    u16 gen = handle_gen(handle);
    u16 *gen_arr = (u16*)p->gens;
    gen_arr[dense_idx] = gen;

    return handle;
}

void fixture_free(struct fixture_pool *p, handle_t handle)
{
    u32 entity_idx = handle_idx(handle);
    assert(entity_idx < MAX_ENTITIES_FOR_SPARSE);
    u16 *sparse = (u16*)p->sparse_from_entity;
    u32 *dense = (u32*)p->dense_to_entity;

    u16 stored = sparse[entity_idx];
    if (stored == 0) return; // NOTE: not a fixture noop

    u32 dead_dense_idx = stored - 1;
    u16 gen = handle_gen(handle);

    u16 *gen_arr = (u16*)p->gens;
    if (gen_arr[dead_dense_idx] != gen) return; // NOTE: stale handle noop

    struct fixture *slots = (struct fixture*)p->slots;
    // TODO: replace with actual fields
    slots[dead_dense_idx].foo = 0;
    slots[dead_dense_idx].bar = 0;
    slots[dead_dense_idx].baz = 0;

    u32 last_dense_idx = p->count - 1;
    if (dead_dense_idx != last_dense_idx)
    {
        slots[dead_dense_idx] = slots[last_dense_idx];
        gen_arr[dead_dense_idx] = gen_arr[last_dense_idx];
        u32 moved_entity_idx = dense[last_dense_idx];
        sparse[moved_entity_idx] = dead_dense_idx + 1;
        dense[dead_dense_idx] = dense[last_dense_idx];
    }
    sparse[entity_idx] = 0;
    p->count--;
}

struct fixture* fixture_ptr(struct fixture_pool *p, handle_t handle)
{
    u32 entity_idx = handle_idx(handle);
    assert(entity_idx < MAX_ENTITIES_FOR_SPARSE);
    u16 *sparse = (u16*)p->sparse_from_entity;
    u16 stored = sparse[entity_idx];
    if (stored == 0) return NULL; // NOTE: non-fixture entity noop

    u16 dense_idx = stored - 1;
    assert(dense_idx < p->count);

    u16 gen = handle_gen(handle);
    u16 *gen_arr = (u16*)p->gens;
    if (gen_arr[dense_idx] != gen) return NULL; // NOTE: stale handle noop

    struct fixture *slots = (struct fixture*)p->slots;
    return &slots[dense_idx];
}
