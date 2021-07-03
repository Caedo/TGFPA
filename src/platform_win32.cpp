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