#ifndef TRANSFORM_POOL_H
#define TRANSFORM_POOL_H

#include "base.h"

struct transform_pool;

struct transform_pool* transform_pool_create(u32 initial_capacity);
void transform_reset(struct transform_pool *t, handle_t handle, u16 new_gen);

#endif
