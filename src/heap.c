#include "heap.h"
#include "base.h"
#include "assert.h"
#include "math.h"

struct heap {
    u8 *ptr;
    u8 *limit;
};
static struct heap heap = {0};

extern i32 memory_grow(i32 pages);
extern i32 wasm_memory_size_pages();
extern unsigned char __heap_base;

static inline uintptr_t align_up_uintptr(uintptr_t x, size_t align)
{
    assert((align & (align - 1)) == 0);
    assert(align > 0);
    // NOTE: bitmask rounds up X to nearest multiple of align
    return (x + (align - 1)) & ~(uintptr_t)(align - 1);
}

static inline u8* align_up_ptr(u8* p, size_t align)
{
    uintptr_t addr = (uintptr_t)p;
    addr = align_up_uintptr(addr, align);
    return (u8*)addr;
}

static inline u64 align_up(u64 x, u64 align)
{
    assert(align && (align & (align - 1)) == 0);
    return (x + (align - 1)) & ~(align - 1);
}

void heap_init(void)
{
    uintptr_t base = (uintptr_t)&__heap_base;
    base = align_up_uintptr(base, 64);
    heap.ptr = (u8*)base;
    heap.limit = (u8*)((uintptr_t)wasm_memory_size_pages() * PAGE_SIZE);
    assert(heap.ptr <= heap.limit);
}

// NOTE: u8 = uint8_t = 1 byte = 8 bits
// We use this for our bump allocator so we can think is raw bytes
// not sizeof(T) chunks. This allow us to use pointers into our
// heap buffer with guaranteed alignment.
u8* heap_alloc(size_t bytes, size_t align)
{
    assert(align && ((align & (align - 1)) == 0));
    u8* p = align_up_ptr(heap.ptr, align);
    uintptr_t paddr = (uintptr_t)p;
    if ((uintptr_t)bytes > UINTPTR_MAX - paddr) return NULL;
    uintptr_t endaddr = paddr + (uintptr_t)bytes;
    if (endaddr > (uintptr_t)heap.limit)
    {
        size_t deficit_bytes = (size_t)(endaddr - (uintptr_t)heap.limit);
        size_t pages = (deficit_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
        pages += MAX(1, pages >> 2);
        if (pages > (size_t)INT32_MAX) pages = (size_t)INT32_MAX;
        i32 old_pages = memory_grow((i32)pages);
        if (old_pages < 0) return NULL;
        heap.limit = (u8*)((uintptr_t)wasm_memory_size_pages() * PAGE_SIZE);
        assert((uintptr_t)heap.ptr <= (uintptr_t)heap.limit);
    }
    heap.ptr = (u8*)endaddr;
    return p;
}

static inline u64 compute_display_stride_bytes(u32 width_pixels, size_t align_bytes)
{
    u64 raw = (u64)width_pixels * (u64)BYTES_PER_PIXEL;
    return align_up(raw, (u64)align_bytes);
}

u8* alloc_display_buffer(u32 width_px, u32 height_px, size_t align_bytes, size_t* out_stride_bytes)
{
    assert(align_bytes && ((align_bytes & (align_bytes - 1)) == 0));
    u64 stride64 = compute_display_stride_bytes(width_px, align_bytes);
    if (stride64 > (u64)SIZE_MAX) return NULL;

    u64 total64 = stride64 * (u64)height_px;
    if (height_px != 0 && stride64 > ((u64)SIZE_MAX / (u64)height_px)) return NULL;

    size_t stride = (size_t)stride64;
    size_t total = (size_t)total64;

    u8* base = heap_alloc(total, MAX(align_bytes,64));
    if (!base) return NULL;

    if (out_stride_bytes) *out_stride_bytes = stride;
    return base;
}
