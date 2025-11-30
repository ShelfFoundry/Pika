#ifndef FIXTURE_POOL_H
#define FIXTURE_POOL_H

#include "base.h"

struct fixture_pool;

struct fixture_pool* fixture_pool_create(u32 initial_capacity);
struct fixture* fixture_alloc(struct fixture_pool *p, handle_t handle);
void fixture_free(struct fixture_pool *p, handle_t handle);
struct fixture* fixture_ptr(struct fixture_pool *p, handle_t handle);

#endif
