#ifndef PARSER_H
#define PARSER_H

enum TypeOfToken { 
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenSquareBrace,
    Token_CloseSquareBrace,
    Token_Comma,
    Token_Equal,
    Token_Semicolon,

    Token_Hash,
    
    Token_Number,
    Token_String,
    Token_Identifier,
    
    Token_Unknown,
    Token_EndOfStream,
};

struct Token { 
    TypeOfToken type;
    
    int length;
    char* text;
};

struct Tokenizer
{
    char* position;
    bool parsing;
    
    bool cacheValid;
    Token cachedToken;
};

struct ShaderInclude {
    char* positionInSource;
    Str8 path;
};

enum UniformType {
    // scalars
    UniformType_Bool,
    UniformType_Int,
    UniformType_Uint,
    UniformType_Float,
    UniformType_Double,

    // vectors
    UniformType_BVec2,
    UniformType_BVec3,
    UniformType_BVec4,

    UniformType_IVec2,
    UniformType_IVec3,
    UniformType_IVec4,

    UniformType_UVec2,
    UniformType_UVec3,
    UniformType_UVec4,

    UniformType_Vec2,
    UniformType_Vec3,
    UniformType_Vec4,

    UniformType_DVec2,
    UniformType_DVec3,
    UniformType_DVec4,

    UniformType_Mat2,
    UniformType_Mat3,
    UniformType_Mat4,

    UniformType_Mat2x2,
    UniformType_Mat2x3,
    UniformType_Mat2x4,

    UniformType_Mat3x2,
    UniformType_Mat3x3,
    UniformType_Mat3x4,

    UniformType_Mat4x2,
    UniformType_Mat4x3,
    UniformType_Mat4x4,
    UniformType_Count
};

char* UniformTypeNames[] {
    // scalars
    "bool",
    "int",
    "uint",
    "float",
    "double",

    // vectors
    "bvec2",
    "bvec3",
    "bvec4",

    "ivec2",
    "ivec3",
    "ivec4",

    "uvec2",
    "uvec3",
    "uvec4",

    "vec2",
    "vec3",
    "vec4",

    "dvec2",
    "dvec3",
    "dvec4",

    "mat2",
    "mat3",
    "mat4",

    "mat2x2",
    "mat2x3",
    "mat2x4",

    "mat3x2",
    "mat3x3",
    "mat3x4",

    "mat4x2",
    "mat4x3",
    "mat4x4",
};

struct ShaderUniformData {
    char name[128];
    UniformType type;
    GLint location;

    bool isArray;
    int arrayLength;

    union {
        float floatValue;
        double doubleValue;
        int intValue;

        float fVectorValue[4];
        int32_t iVectorValue[4];
        uint32_t uVectorValue[4];
    };
};

#endif //PARSER_H