#include <windows.h>

FILETIME GetLastWriteTime(char* filename) {
    FILETIME ret = {};

    WIN32_FIND_DATA findData;
    HANDLE findHandle = FindFirstFile(filename, &findData);
    if(findHandle != INVALID_HANDLE_VALUE) {
        FindClose(findHandle);
        ret = findData.ftLastWriteTime;
    }

    return ret;
}

void FillOFNStruct(OPENFILENAME* ofn, FileType fileType) {
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

char* OpenFileDialog(MemoryArena* arena, FileType fileType) {
    assert(arena);
    assert(arena->baseAddres);
    
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle

    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = (LPSTR) PushArena(arena, MAX_PATH);
    
    // Make sure, that path is an empty string
    ofn.lpstrFile[0] = '\0';

    FillOFNStruct(&ofn, fileType);

    // if(fileName != nullptr) {
    //     ofn.lpstrFileTitle = (LPSTR) fileName->str;
    //     ofn.nMaxFileTitle = (DWORD) fileName->capacity;
    // }
    
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Display the Open dialog box. 
    BOOL result = GetOpenFileName(&ofn);
    if(result) {
        return ofn.lpstrFile;
    }
    
    return NULL;
}

Str8 SaveFileDialog(MemoryArena* arena, FileType fileType) {
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

    FillOFNStruct(&ofn, fileType);

    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    
    // Display the Open dialog box. 
    BOOL result = GetSaveFileName(&ofn);
    if(result) {
        return Str8{ofn.lpstrFile, strlen(ofn.lpstrFile)};
    }
    
    return {};
}
