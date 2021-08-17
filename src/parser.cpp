
void Error(Tokenizer* tokenizer, Token* token, char* message) {
    tokenizer->parsing = false;
    if(token)
        fprintf(stderr, "[ERROR] %s \n Given token: %s\n", message, token->text);
    else 
        fprintf(stderr, "[ERROR] %s \n", message);
}

inline bool IsWhiteSpace(char c) {
    return c == ' '  ||
           c == '\n' ||
           c == '\t' ||
           c == '\r';
}

inline bool IsEndOfLine(char c) {
    return c == '\n' || c == '\r';
}

inline bool IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z');
}

inline bool IsNumber(char c) {
    return (c >= '0' && c <= '9');
}

void EatWhiteSpaces(Tokenizer* tokenizer) {
    while(IsWhiteSpace(*tokenizer->position))
        tokenizer->position++;
}

Token PeekNextToken(Tokenizer* tokenizer) {
    EatWhiteSpaces(tokenizer);
    
    char firstChar = tokenizer->position[0];
    
    Token token = {};
    token.length = 1;
    token.text = tokenizer->position;
    
    char* at = tokenizer->position;
    
    at++;
    switch (firstChar)
    {
        case '{':  token.type = Token_OpenBrace;          break;
        case '}':  token.type = Token_CloseBrace;         break;
        case '(':  token.type = Token_OpenBracket;        break;
        case ')':  token.type = Token_CloseBracket;       break;
        case '[':  token.type = Token_OpenSquareBracket;  break;
        case ']':  token.type = Token_CloseSquareBracket; break;
        case '=':  token.type = Token_Equal;              break;
        case ',':  token.type = Token_Comma;              break;
        case '#':  token.type = Token_Hash;               break;
        case ';':  token.type = Token_Semicolon;          break;
        
        case '\0': {
            token.type = Token_EndOfStream;
            tokenizer->parsing = false;
        } 
        break;
        
        case '"': {
            token.type = Token_String;
            while(at[0] != '"') {
                ++at;
            }
            ++at;
            
            token.length = (int) (at - token.text);
        }
        break;
        
        default: {
            if(IsAlpha(firstChar)) {
                token.type = Token_Identifier;
                
                while(IsAlpha(at[0]) ||
                      IsNumber(at[0]) ||
                      at[0] == '_')
                {
                    ++at;
                }
                
                token.length = (int) (at - token.text);
            }
            else if(IsNumber(firstChar) || firstChar == '-') {
                token.type = Token_Number;
                
                while(IsNumber(at[0]) ||
                      at[0] == '.')
                {
                    ++at;
                }
                
                token.length = (int) (at - token.text);
            }
            else {
                token.type = Token_Unknown;
                while(IsEndOfLine(at[0]) == false)
                    ++at;
                
                token.length = (int) (at - token.text);
            }
        }
        break;
    }
    
    tokenizer->cacheValid = true;
    tokenizer->cachedToken = token;
    
    return token;
}

Token GetNextToken(Tokenizer* tokenizer) {
    
    Token token;
    if(tokenizer->cacheValid == false) {
        token = PeekNextToken(tokenizer);
    }
    else {
        token = tokenizer->cachedToken;
    }
    
    tokenizer->position = token.text + token.length;
    tokenizer->cacheValid = false;
    
    return token;
}

Token RequireToken(Tokenizer* tokenizer, TypeOfToken expectedType) {
    Token token = GetNextToken(tokenizer);
    if(token.type != expectedType) {
        Error(tokenizer, &token, "Unexpected token");
    }
    
    return token;
}

bool OptionalToken(Tokenizer* tokenizer, TypeOfToken optionalType) {
    Token token = GetNextToken(tokenizer);
    if(token.type != optionalType) {
        return false;
    }
    
    return true;
}

bool IsTokenEqual(Token token, char* match) {
    char* at = match;
    for (int i = 0; i < token.length; i++)
    {
        if(*at == 0 || token.text[i] != *at) {
            return false;
        }
        
        at++;
    }
    
    return *at == 0;
}

bool TokenContains(Token token, Str8 str) {
    for(int i = 0; i < token.length; i++) {

        bool found = true;
        for(int j = 0; j < str.length; j++) {
            if(token.text[i] != str.string[j]) {
                found = false;
                break;
            }

            i++;
            if(i >= token.length)
                return false;
        }

        if(found)
            return true;
    }

    return false;
}

void StringCopy(char* dest, char* source, int length) {
    for(int i = 0; i < length; i++) {
        dest[i] = source[i];
    }
}

float GetFloat(Token token) {
    if(token.length == 0) 
        return 0;
    
    float result = (float) atof(token.text);
    return result;
}

ShaderLib* GetShaderLibs(char* shaderSource, MemoryArena* arena, int* libsCount) {
    Tokenizer tokenizer = {};

    tokenizer.position = shaderSource;
    tokenizer.parsing = true;

    *libsCount = 0;

    ShaderLib* ret = (ShaderLib*) ((char*)arena->baseAddres + arena->allocatedOffset);

    while(tokenizer.parsing) {
        Token token = GetNextToken(&tokenizer);
        if(token.type == Token_Identifier) {
            if(IsTokenEqual(token, "LIB")) {
                RequireToken(&tokenizer, Token_OpenBracket);
                Token nameToken = RequireToken(&tokenizer, Token_String);
                RequireToken(&tokenizer, Token_CloseBracket);

                if(tokenizer.parsing == false)
                    return NULL;

                ShaderLib* lib = (ShaderLib*) PushArena(arena, sizeof(ShaderLib));

                lib->name.string = nameToken.text + 1;
                lib->name.length = nameToken.length - 2;

                (*libsCount)++;
            }
        }
    }

    return ret;
}

void GetTypeData(Token token, UniformScalarType* type, int* vectorLength) {
    assert(token.type == Token_Identifier);

    if(IsTokenEqual(token, "bool")) {
        *type = UniformType_Bool;
        *vectorLength = 1;
    }
    else if(IsTokenEqual(token, "int")) {
        *type = UniformType_Int;
        *vectorLength = 1;
    }
    else if(IsTokenEqual(token, "uint")) {
        *type = UniformType_UInt;
        *vectorLength = 1;
    }
    else if(IsTokenEqual(token, "float")) {
        *type = UniformType_Float;
        *vectorLength = 1;
    }
    else if(IsTokenEqual(token, "double")) {
        *type = UniformType_Double;
        *vectorLength = 1;
    }
    else if(TokenContains(token, Str8Lit("vec"))) {
        switch(token.text[0]) {
            case 'b': *type = UniformType_Bool;   break;
            case 'i': *type = UniformType_Int;    break;
            case 'u': *type = UniformType_UInt;   break;
            case 'v': *type = UniformType_Float;  break;
            case 'd': *type = UniformType_Double; break;
            default: fprintf(stderr, "SyntaxError..."); break; // TODO: Better errors
        }

        switch(token.text[token.length - 1]) {
            case '2': *vectorLength = 2; break;
            case '3': *vectorLength = 3; break;
            case '4': *vectorLength = 4; break;
            default: fprintf(stderr, "SyntaxError..."); break; // TODO: Better errors
        }
    }
    else if(TokenContains(token, Str8Lit("mat"))) {
        *type = UniformType_Matrix;
    }
}

void GetNumber(Token token, void* dest, UniformScalarType type) {
    switch(type) {
        case UniformType_Int: {
            int value = (int) GetFloat(token);
            *((int*) dest) = value;
        }
        break;

        case UniformType_Float: {
            float value = GetFloat(token);
            *((float*) dest) = value;
        }
        break;

        case UniformType_UInt: {
            uint32_t value = (uint32_t) GetFloat(token);
            *((uint32_t*) dest) = value;
        }
        break;

        case UniformType_Double: {
            double value = (double) GetFloat(token);
            *((double*) dest) = value;
        }
        break;

        case UniformType_Bool: {
            bool value = (bool) GetFloat(token);
            *((bool*) dest) = value;
        }
        break;

        default: {
            // TODO: Better errors
            fprintf(stderr, "Not handled type in GetNumber()");
        }
    }
}

ShaderUniformData* GetShaderUniforms(char* shaderSource, MemoryArena* arena, int* count) {
    Tokenizer tokenizer = {};

    tokenizer.position = shaderSource;
    tokenizer.parsing = true;

    ShaderUniformData* ret = (ShaderUniformData*) ((char*)arena->baseAddres + arena->allocatedOffset);

    *count = 0;

    // TODO: uniform metadata struct?
    int attribFlags = 0;
    float minRangeValue = 0;
    float maxRangeValue = 0;

    while(tokenizer.parsing) {
        Token token = GetNextToken(&tokenizer);

        if(token.type == Token_Identifier)
        {
            if(IsTokenEqual(token, "uniform")) {
                Token typeToken = RequireToken(&tokenizer, Token_Identifier);
                Token nameToken = RequireToken(&tokenizer, Token_Identifier);

                if(tokenizer.parsing == false)
                    return NULL;

                ShaderUniformData* uniform = (ShaderUniformData*) PushArena(arena, sizeof(ShaderUniformData));

                StringCopy(uniform->name, nameToken.text, nameToken.length + 1);
                uniform->name[nameToken.length] = 0;

                GetTypeData(typeToken, &uniform->type, &uniform->vectorLength);

                Token nextToken = PeekNextToken(&tokenizer);
                if(nextToken.type == Token_OpenSquareBracket) {
                    GetNextToken(&tokenizer);
                    Token lengthToken = RequireToken(&tokenizer, Token_Number);
                    RequireToken(&tokenizer, Token_CloseSquareBracket);

                    if(tokenizer.parsing == false) {
                        return NULL;
                    }

                    uniform->isArray = true;
                    uniform->arrayLength = atoi(lengthToken.text);
                }
                else if(nextToken.type == Token_Equal) {
                    GetNextToken(&tokenizer);
                    nextToken = PeekNextToken(&tokenizer);
                    if(nextToken.type == Token_Identifier) {
                        // parse default types with specified type, for example:
                        // vec4(1,1,1,1), float(2.0), etc.
                        GetNextToken(&tokenizer);

                        UniformScalarType defaultValueType;
                        int defaultValueLength;

                        GetTypeData(nextToken, &defaultValueType, &defaultValueLength);

                        if(defaultValueLength != uniform->vectorLength ||
                           defaultValueType != uniform->type) {
                            tokenizer.parsing = false;
                            break;
                        }

                        RequireToken(&tokenizer, Token_OpenBracket);

                        for(int i = 0; i < 4; i++) {
                            Token valueToken = GetNextToken(&tokenizer);
                            if(valueToken.type == Token_Number) {
                                // TODO: will not work for doubles
                                GetNumber(valueToken, (void*) &(uniform->floatValue[i]), defaultValueType);

                            }
                            else {
                                break;
                            }

                            nextToken = PeekNextToken(&tokenizer);
                            if(nextToken.type == Token_CloseBracket) {
                                break;
                            }
                            else {
                                GetNextToken(&tokenizer);
                            }
                        }

                        RequireToken(&tokenizer, Token_CloseBracket);
                    }
                    else if(nextToken.type == Token_Number) {
                        // parse default types for scalars

                        Token valueToken = GetNextToken(&tokenizer);
                        GetNumber(valueToken, (void*) uniform->floatValue, uniform->type);
                    }
                }

                uniform->location = -1;

                uniform->minRangeValue = minRangeValue;
                uniform->maxRangeValue = maxRangeValue;
                uniform->attributeFlags = attribFlags;

                attribFlags = UniformAttribFlag_None;
                minRangeValue = 0;
                maxRangeValue = 0;

                (*count)++;
            }
            else if(IsTokenEqual(token, "COLOR")) {
                RequireToken(&tokenizer, Token_OpenBracket);
                RequireToken(&tokenizer, Token_CloseBracket);

                if(tokenizer.parsing) {
                    attribFlags |= UniformAttribFlag_Color;
                }
                else {
                    // TODO: Better errors
                    fprintf(stderr, "SyntaxError...");
                }
            }
            else if(IsTokenEqual(token, "RANGE")) {
                RequireToken(&tokenizer, Token_OpenBracket);
                Token minToken = RequireToken(&tokenizer, Token_Number);
                RequireToken(&tokenizer, Token_Comma);
                Token maxToken = RequireToken(&tokenizer, Token_Number);
                RequireToken(&tokenizer, Token_CloseBracket);

                if(tokenizer.parsing) {
                    minRangeValue = GetFloat(minToken);
                    maxRangeValue = GetFloat(maxToken);

                    attribFlags |= UniformAttribFlag_Range;
                }
            }
        }
    }

    return ret;
}