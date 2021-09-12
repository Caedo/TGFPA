#ifndef MEMORY_MANAGEMENT
#define MEMORY_MANAGEMENT

// Based on Ryan Fleury's implementation: https://github.com/ryanfleury/app_template/blob/master/source/memory.h

#define ARENA_SIZE_MAX Gigabytes(1)
#define ARENA_COMMIT_SIZE Kilobytes(4);

struct MemoryArena {
    void* baseAddres;
    uint64_t allocatedOffset;
    uint64_t commitedOffset;
};

void* PushArena(MemoryArena* arena, uint64_t size);

#endif