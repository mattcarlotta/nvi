#include "arena.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ARENA_ASAN 1
#endif
#elif defined(__SANITIZE_ADDRESS__)
#define ARENA_ASAN 1
#endif

#ifdef ARENA_ASAN
#include <sanitizer/asan_interface.h>
// gcc's -Wmaybe-uninitialized assumes the const pointer parameter implies a read and flags
// poisoning freshly malloc'd chunk memory; poisoning only tags shadow bytes, so silence it
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#define ARENA_POISON(addr, size) __asan_poison_memory_region((addr), (size))
#define ARENA_UNPOISON(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#define ARENA_POISON(addr, size) ((void)0)
#define ARENA_UNPOISON(addr, size) ((void)0)
#endif

#define ARENA_ALIGN_UP(n) (((n) + (ARENA_ALIGNMENT - 1)) & ~(uintptr_t)(ARENA_ALIGNMENT - 1))

static void arena_oom(void) {
    fprintf(stderr, "[ERROR] Failed to allocate memory (system out of memory?); aborting.\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
}

static arena_chunk_t *arena_new_chunk(size_t capacity) {
    if (capacity > SIZE_MAX - sizeof(arena_chunk_t)) {
        arena_oom();
    }

    arena_chunk_t *chunk = malloc(sizeof(arena_chunk_t) + capacity);
    if (chunk == NULL) {
        arena_oom();
    }

    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->offset = 0;

    ARENA_POISON(chunk->data, capacity);

    return chunk;
}

static char *arena_chunk_bump(arena_chunk_t *chunk, size_t size) {
    uintptr_t base = (uintptr_t)chunk->data;
    uintptr_t aligned = ARENA_ALIGN_UP(base + chunk->offset);
    uintptr_t end = base + chunk->capacity;

    if (aligned > end || size > end - aligned) {
        return NULL;
    }

    chunk->offset = (aligned + size) - base;

    ARENA_UNPOISON((void *)aligned, size);

    return (char *)aligned;
}

void arena_init(arena_t *arena, size_t first_chunk_size) {
    arena->head = NULL;
    arena->next_chunk_size = first_chunk_size != 0 ? first_chunk_size : ARENA_DEFAULT_CHUNK_SIZE;
    arena->last_alloc = NULL;
    arena->last_size = 0;
}

void *arena_alloc(arena_t *arena, size_t size) {
    if (arena->head != NULL) {
        char *p = arena_chunk_bump(arena->head, size);
        if (p != NULL) {
            arena->last_alloc = p;
            arena->last_size = size;
            return p;
        }
    }

    if (size > SIZE_MAX - ARENA_ALIGNMENT) {
        arena_oom();
    }

    // ARENA_ALIGNMENT of headroom guarantees the aligned request fits in a fresh chunk
    if (size + ARENA_ALIGNMENT > arena->next_chunk_size) {
        // Oversized request: dedicated exactly-sized chunk spliced in behind the current
        // chunk so its remaining space stays usable and the doubling schedule is unaffected
        arena_chunk_t *chunk = arena_new_chunk(size + ARENA_ALIGNMENT);
        if (arena->head != NULL) {
            chunk->next = arena->head->next;
            arena->head->next = chunk;
        } else {
            arena->head = chunk;
        }

        char *p = arena_chunk_bump(chunk, size);
        arena->last_alloc = p;
        arena->last_size = size;
        return p;
    }

    arena_chunk_t *chunk = arena_new_chunk(arena->next_chunk_size);
    chunk->next = arena->head;
    arena->head = chunk;

    if (arena->next_chunk_size < ARENA_MAX_CHUNK_SIZE) {
        size_t doubled = arena->next_chunk_size * 2;
        arena->next_chunk_size = doubled < ARENA_MAX_CHUNK_SIZE ? doubled : ARENA_MAX_CHUNK_SIZE;
    }

    char *p = arena_chunk_bump(chunk, size);
    arena->last_alloc = p;
    arena->last_size = size;
    return p;
}

void *arena_alloc_zeroed(arena_t *arena, size_t size) {
    void *p = arena_alloc(arena, size);
    memset(p, 0, size);
    return p;
}

void *arena_memdup(arena_t *arena, const void *src, size_t size) {
    void *p = arena_alloc(arena, size);
    memcpy(p, src, size);
    return p;
}

char *arena_strdup(arena_t *arena, const char *s) { return arena_memdup(arena, s, strlen(s) + 1); }

char *arena_strndup(arena_t *arena, const char *s, size_t n) {
    size_t len = 0;
    while (len < n && s[len] != '\0') {
        ++len;
    }

    char *copy = arena_alloc(arena, len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

char *arena_sprintf(arena_t *arena, const char *fmt, ...) {
    if (fmt == NULL) {
        arena_oom();
    }

    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        arena_oom();
    }

    char *p = arena_alloc(arena, (size_t)len + 1);

    va_start(args, fmt);
    vsnprintf(p, (size_t)len + 1, fmt, args);
    va_end(args);

    return p;
}

void *arena_extend(arena_t *arena, void *ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) {
        return ptr;
    }

    if (ptr != NULL && ptr == arena->last_alloc) {
        arena_chunk_t *chunk = arena->head;
        uintptr_t base = (uintptr_t)chunk->data;
        uintptr_t alloc_end = (uintptr_t)ptr + old_size;

        if (alloc_end == base + chunk->offset && new_size - old_size <= chunk->capacity - chunk->offset) {
            ARENA_UNPOISON((char *)alloc_end, new_size - old_size);
            chunk->offset += new_size - old_size;
            arena->last_size = new_size;
            return ptr;
        }
    }

    void *p = arena_alloc(arena, new_size);
    if (ptr != NULL) {
        memcpy(p, ptr, old_size);
    }

    return p;
}

void arena_reset(arena_t *arena) {
    if (arena->head == NULL) {
        return;
    }

    arena_chunk_t *chunk = arena->head->next;
    while (chunk != NULL) {
        arena_chunk_t *next = chunk->next;
        ARENA_UNPOISON(chunk->data, chunk->capacity);
        free(chunk);
        chunk = next;
    }

    arena->head->next = NULL;
    arena->head->offset = 0;
    arena->last_alloc = NULL;
    arena->last_size = 0;

    ARENA_POISON(arena->head->data, arena->head->capacity);
}

void arena_free(arena_t *arena) {
    arena_chunk_t *chunk = arena->head;
    while (chunk != NULL) {
        arena_chunk_t *next = chunk->next;
        ARENA_UNPOISON(chunk->data, chunk->capacity);
        free(chunk);
        chunk = next;
    }

    arena->head = NULL;
    arena->last_alloc = NULL;
    arena->last_size = 0;
}
