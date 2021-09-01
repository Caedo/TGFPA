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