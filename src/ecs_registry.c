#include "base.h"
#include "heap.h"
#include "assert.h"
#include "ecs_registry.h"
#include <stdalign.h>

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
}

struct entity_registry* registry_create(u32 initial_capacity)
{
    struct entity_registry* r = (struct entity_registry*)heap_alloc(sizeof *r, alignof(struct entity_registry));
    registry_init(r, initial_capacity);
    return r;
}

handle_t registry_alloc(struct entity_registry *r)
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

u16 registry_free(struct entity_registry *r, u32 handle)
{
    u32 idx = handle_idx(handle);
    assert(idx < r->capacity);
    u16 gen = handle_gen(handle);

    struct entity *slots = (struct entity*)r->slots;
    if (slots[idx].gen != gen) return 0; // NOTE: stale handle noop

    //flags32 m = slots[idx].mask;
    // TODO: destroy components based on mask before reset
    slots[idx].mask = 0;

    slots[idx].gen++;
    if (slots[idx].gen == 0) return 0; // NOTE: tombstoned

    u32 *next = (u32*)r->next_free;
    next[idx] = r->free_head;
    r->free_head = idx;

    return slots[idx].gen;
}
