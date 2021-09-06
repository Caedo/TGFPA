#define Bytes(n) n
#define Kilobytes(n) (1024 * (uint64_t)(n))
#define Megabytes(n) (1024 * (uint64_t)Kilobytes(n))
#define Gigabytes(n) (1024 * (uint64_t)Megabytes(n))

struct Str8 {
    char* string;
    uint64_t length;
};

#define Str8Lit(c) Str8{c, strlen(c)}

enum FileType {
    FileType_Shader,
    FileType_Image,
};

Str8 GetFileNameFromPath(Str8 path) {

    char* at = path.string + path.length - 1;
    while(at != path.string) {
        if(*at == '\\' || *at == '/') {
            at++;
            break;
        }

        at--;
    }

    return Str8{ at, (uint64_t) (path.string + path.length - at) };
}