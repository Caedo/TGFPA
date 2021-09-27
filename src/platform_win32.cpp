#include <windows.h>

FILETIME Win32_GetLastWriteTime(Str8 filePath) {
    FILETIME ret = {};

    WIN32_FIND_DATA findData;
    HANDLE findHandle = FindFirstFile(filePath.string, &findData);
    if(findHandle != INVALID_HANDLE_VALUE) {
        FindClose(findHandle);
        ret = findData.ftLastWriteTime;
    }

    return ret;
}

void Win32_UpdateLastWriteTime(FileData* fileData) {
    fileData->lastWriteTime = Win32_GetLastWriteTime(fileData->path);
}

bool Win32_FileHasChanged(FileData fileData) {
    FILETIME currentModifyTime = Win32_GetLastWriteTime(fileData.path);
    return CompareFileTime(&currentModifyTime, &fileData.lastWriteTime) != 0;
}

void Win32_FillOFNStruct(OPENFILENAME* ofn, FileType fileType) {
    switch(fileType) {
        case FileType_Shader: {
            ofn->lpstrFilter = "GLSL file (.glsl)\0*.glsl\0";
            ofn->lpstrDefExt = ".glsl";
        }
        break;

        case FileType_Image: {
            ofn->lpstrFilter = "PNG file (.png)\0*.png\0";
            ofn->lpstrDefExt = ".png";
        }
    }
}

Str8 Win32_OpenFileDialog(MemoryArena* arena, FileType fileType) {
    assert(arena);
    assert(arena->baseAddres);
    
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle

    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = (LPSTR) PushArena(arena, MAX_PATH);
    
    // Make sure, that path is an empty string
    ofn.lpstrFile[0] = '\0';

    Win32_FillOFNStruct(&ofn, fileType);

    // if(fileName != nullptr) {
    //     ofn.lpstrFileTitle = (LPSTR) fileName->str;
    //     ofn.nMaxFileTitle = (DWORD) fileName->capacity;
    // }
    
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Display the Open dialog box. 
    BOOL result = GetOpenFileName(&ofn);
    if(result) {
        return Str8{ofn.lpstrFile, strlen(ofn.lpstrFile)};
    }
    
    return {};
}

Str8 Win32_SaveFileDialog(MemoryArena* arena, FileType fileType) {
    assert(arena);
    assert(arena->baseAddres);
    
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle
    ofn.lpstrFile = (LPSTR) PushArena(arena, MAX_PATH);
    
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    
    ofn.nMaxFile = MAX_PATH;

    Win32_FillOFNStruct(&ofn, fileType);

    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    
    // Display the Open dialog box. 
    BOOL result = GetSaveFileName(&ofn);
    if(result) {
        return Str8{ofn.lpstrFile, strlen(ofn.lpstrFile)};
    }
    
    return {};
}

Platform Win32_Initialize() {
    Platform platform = {};

    platform.OpenFileDialog = Win32_OpenFileDialog;
    platform.SaveFileDialog = Win32_SaveFileDialog;

    platform.FileHasChanged = Win32_FileHasChanged;
    platform.UpdateLastWriteTime = Win32_UpdateLastWriteTime;

    return platform;
}