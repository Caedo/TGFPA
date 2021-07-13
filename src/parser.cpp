
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
        case '{':  token.type = Token_OpenBrace;  break;
        case '}':  token.type = Token_CloseBrace; break;
        case '=':  token.type = Token_Equal;      break;
        case ',':  token.type = Token_Comma;      break;
        case '#':  token.type = Token_Hash;       break;
        case ';':  token.type = Token_Semicolon;  break;
        
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
            else if(IsNumber(firstChar)) {
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

void StringCopy(char* dest, char* source, int length) {
    for(int i = 0; i < length; i++) {
        dest[i] = source[i];
    }
}

ShaderInclude* GetShaderIncludes(char* shaderSource, MemoryArena* arena, int* includesCount) {
    Tokenizer tokenizer = {};

    tokenizer.position = shaderSource;
    tokenizer.parsing = true;

    ShaderInclude* ret = (ShaderInclude*) ((char*)arena->baseAddres + arena->allocatedOffset);

    while(tokenizer.parsing) {
        Token token = GetNextToken(&tokenizer);
        if(token.type == Token_Hash) {
            Token nextToken = GetNextToken(&tokenizer);
            if(IsTokenEqual(nextToken, "include")) {
                Token pathToken = RequireToken(&tokenizer, Token_String);

                if(tokenizer.parsing == false)
                    return NULL;

                ShaderInclude* include = (ShaderInclude*) PushArena(arena, sizeof(ShaderInclude));

                include->positionInSource = token.text;
                include->path.string = pathToken.text + 1;
                include->path.length = pathToken.length - 2;

                (*includesCount)++;
            }
        }
    }

    return ret;
}


ShaderUniformData* GetShaderUniforms(char* shaderSource, MemoryArena* arena, int* count) {
    Tokenizer tokenizer = {};

    tokenizer.position = shaderSource;
    tokenizer.parsing = true;

    ShaderUniformData* ret = (ShaderUniformData*) ((char*)arena->baseAddres + arena->allocatedOffset);

    *count = 0;

    while(tokenizer.parsing) {
        Token token = GetNextToken(&tokenizer);

        if(token.type == Token_Identifier &&
           IsTokenEqual(token, "uniform"))
        {
            Token typeToken = RequireToken(&tokenizer, Token_Identifier);
            Token nameToken = RequireToken(&tokenizer, Token_Identifier);

            if(tokenizer.parsing == false)
                return NULL;

            ShaderUniformData* uniform = (ShaderUniformData*) PushArena(arena, sizeof(ShaderUniformData));

            if(IsTokenEqual(typeToken, "int")) {
                uniform->type = Int;
            }
            else if(IsTokenEqual(typeToken, "float")) {
                uniform->type = Float;
            }
            else {
                // PopArena(arena, sizeof(ShaderUniformData));
                // continue;
            }

            StringCopy(uniform->name, nameToken.text, nameToken.length + 1);
            uniform->name[nameToken.length] = 0;

            uniform->location = -1;

            (*count)++;
        }
    }

    return ret;
}