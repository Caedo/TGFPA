#ifndef PLATFORM_H
#define PLATFORM_H

struct Platform {
    Str8 (*OpenFileDialog)(MemoryArena* arena, FileType fileType);
    Str8 (*SaveFileDialog)(MemoryArena* arena, FileType fileType);

    bool (*FileHasChanged)(FileData fileData);
    void (*UpdateLastWriteTime)(FileData* fileData);
};

#endif