#ifndef PARSER_H
#define PARSER_H

enum TypeOfToken { 
    Token_OpenBrace,
    Token_CloseBrace,
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
    Int,
    Float,
};

struct ShaderUniformData {
    char name[128];
    UniformType type;
    GLint location;

    union {
        float floatValue;
        int intValue;
    };
};

#endif //PARSER_H