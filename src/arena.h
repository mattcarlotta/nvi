#ifndef ARENA_H
#define ARENA_H

// Chunked bump allocator. Allocations are O(1) pointer bumps out of malloc'd chunks and are
// never freed individually; the entire arena is released (or rewound) at once. Intended
// lifetimes:
//   - parser: everything allocated while parsing .env files lives until exec, so one
//     arena_free (or nothing at all, pre-exec) replaces per-string cleanup
//   - scanner: one persistent findings arena per worker thread (no locking), plus an
//     optional scratch arena that is arena_reset between files for zero steady-state
//     allocator traffic
//
// Growth: the first chunk defaults to ARENA_DEFAULT_CHUNK_SIZE (overridable per arena via
// arena_init). Each subsequent chunk doubles, capped at ARENA_MAX_CHUNK_SIZE. A request
// larger than the next chunk size gets its own exactly-sized chunk, spliced in behind the
// current chunk so the current chunk's remaining space stays usable.
//
// OOM matches dynarr.h: print to stderr and exit(EXIT_FAILURE).
//
// AddressSanitizer: when built under ASan, the unused tail of every chunk is poisoned and
// alignment padding between allocations is left poisoned as small redzones, so intra-arena
// overflows still trip ASan/libFuzzer instead of silently landing in valid arena memory.

#include <stddef.h>

// 16 covers max_align_t on all three targets without relying on C11 _Alignof under older MSVC
#define ARENA_ALIGNMENT 16

#define ARENA_DEFAULT_CHUNK_SIZE (64 * 1024)
#define ARENA_MAX_CHUNK_SIZE (1024 * 1024)

typedef struct arena_chunk arena_chunk_t;
typedef struct arena arena_t;

struct arena_chunk {
    arena_chunk_t *next;
    size_t capacity;
    size_t offset;
    char data[];
};

struct arena {
    arena_chunk_t *head;    // current bump chunk (largest under the doubling schedule)
    size_t next_chunk_size; // capacity of the next chunk to allocate
    char *last_alloc;       // most recent allocation, enables arena_extend in place
    size_t last_size;
};

// Initializes an arena. No memory is allocated until the first arena_alloc.
// A first_chunk_size of 0 uses ARENA_DEFAULT_CHUNK_SIZE. When the input size is known up
// front (fstat), seeding with something like file_size * 2 + 16K makes a second chunk
// nearly impossible.
void arena_init(arena_t *arena, size_t first_chunk_size);

// Returns a pointer to `size` bytes aligned to ARENA_ALIGNMENT. Never returns NULL
// (aborts on OOM, matching dynarr.h). A size of 0 returns a valid unique pointer.
void *arena_alloc(arena_t *arena, size_t size);

// arena_alloc followed by memset(0)
void *arena_alloc_zeroed(arena_t *arena, size_t size);

// Copies `size` bytes of `src` into the arena
void *arena_memdup(arena_t *arena, const void *src, size_t size);

// Copies a NUL-terminated string into the arena
char *arena_strdup(arena_t *arena, const char *s);

// Copies at most `n` bytes of `s` into the arena and NUL-terminates
char *arena_strndup(arena_t *arena, const char *s, size_t n);

// printf-style formatting into the arena; returns the NUL-terminated result
char *arena_sprintf(arena_t *arena, const char *fmt, ...);

// Grows `ptr` from `old_size` to `new_size`. If `ptr` is the arena's most recent allocation
// and the current chunk has room, it grows in place; otherwise the contents are copied into
// a fresh allocation. Useful for building interpolated values incrementally without a
// separate scratch buffer. Returns the (possibly moved) pointer.
void *arena_extend(arena_t *arena, void *ptr, size_t old_size, size_t new_size);

// Rewinds the arena to empty without releasing its largest chunk, so repeated
// fill/reset cycles (per-file scratch space) converge to zero malloc traffic.
// All other chunks are freed. Pointers previously returned become invalid.
void arena_reset(arena_t *arena);

// Frees every chunk. The arena can be reused after another arena_init.
void arena_free(arena_t *arena);

#endif
