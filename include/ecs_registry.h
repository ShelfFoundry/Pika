#ifndef ECS_REGISTRY_H
#define ECS_REGISTRY_H

#include "base.h"

enum { IDX_BITS = 20, GEN_BITS = 12 };
enum { IDX_MASK = ((1u << IDX_BITS) - 1u),
       GEN_MASK = ((1u << GEN_BITS) - 1u),
       GEN_SHIFT = IDX_BITS };
enum { NULL_HANDLE = 0u, INVALID_U32 = 0xFFFFFFFFu };
enum {
    COMPONENT_TRANSFORM = 1u << 0,
    COMPONENT_FIXTURE   = 1u << 1,
    COMPONENT_POSITION  = 1u << 2,
    COMPONENT_PRODUCT   = 1u << 3,
};


static inline handle_t handle_pack(u32 idx, u16 gen)
{
    // NOTE: (idx & IDX_MASK): keep only low 20 bits
    // ((u32)gen & GEN_MASK) << GEN_SHIFT: place gen in upper 12 bits
    return (idx & IDX_MASK) | (((u32)gen & GEN_MASK) << GEN_SHIFT);
}
static inline u32 handle_idx(handle_t handle){ return handle & IDX_MASK; }
static inline u16 handle_gen(handle_t handle){ return (u16)(handle >> GEN_SHIFT) & GEN_MASK; }

struct entity_registry;

struct entity_registry* registry_create(u32 initial_capacity);
handle_t registry_alloc(struct entity_registry *r);
u16 registry_free(struct entity_registry *r, u32 handle);

handle_t entity_attach_fixture(struct entity_registry *r, handle_t handle);
handle_t entity_detach_fixture(struct entity_registry *r, handle_t handle);

#endif
