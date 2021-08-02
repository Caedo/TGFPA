#ifndef PARSER_H
#define PARSER_H

enum TypeOfToken { 
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenSquareBracket,
    Token_CloseSquareBracket,
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

struct ShaderLib {
    Str8 name;
};

enum UniformScalarType {
    UniformType_Bool,
    UniformType_Int,
    UniformType_UInt,
    UniformType_Float,
    UniformType_Double,
    UniformType_Matrix,
    UniformType_Count,
};

enum UniformAttribFlag {
    UniformAttribFlag_None = 0,
    UniformAttribFlag_Color = 1,
    UniformAttribFlag_Range = 2,
};

struct ShaderUniformData {
    char name[128];
    GLint location;

    int vectorLength;

    int matrixWidth;
    int matrixHeight;

    bool isArray;
    int arrayLength;

    int attributeFlags;

    float minRangeValue;
    float maxRangeValue;

    UniformScalarType type;
    union {
        float    floatValue[4];
        double   doubleValue[4];
        int32_t  intValue[4];
        uint32_t uintValue[4];
    };
};

#endif //PARSER_H