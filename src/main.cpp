#include <stdio.h>
#include <ctype.h>

#include "glad.c"

#include "ImGUI/imgui.cpp"
#include "ImGUI/imgui_draw.cpp"
#include "ImGUI/imgui_tables.cpp"
#include "ImGUI/imgui_widgets.cpp"
#include "ImGUI/imgui_impl_glfw.cpp"
#include "ImGUI/imgui_impl_opengl3.cpp"

#include "ImGUI/imgui_demo.cpp"

#define GLFW_DLL
#define GLFW_INCLUDE_NONE

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#define Bytes(n) n
#define Kilobytes(n) (1024 * (uint64_t)(n))
#define Megabytes(n) (1024 * (uint64_t)Kilobytes(n))
#define Gigabytes(n) (1024 * (uint64_t)Megabytes(n))

struct Str8 {
    char* string;
    uint64_t length;
};

#define Str8Lit(c) Str8{c, strlen(c)}

#include "memory_management.h"
#include "parser.h"

#include "platform_win32.cpp"
#include "memory_management.cpp"
#include "parser.cpp"

MemoryArena temporaryArena;

struct Shader {
    MemoryArena shaderMemory;

    Str8 source;
    Str8 filePath;

    bool isValid;

    GLuint handle;
    GLint resolutionLocation;
    GLint timeLocation;
    GLint timeDeltaLocation;
    GLint frameLocation;

    int uniformsCount;
    ShaderUniformData* uniforms;

    int libsCount;
    ShaderLib* libs;

    // @Win32
    FILETIME lastWriteTime;
};

struct Framebuffer {
    GLuint handle;
    GLuint colorTexture;

    int width;
    int height;
};

bool show_demo_window;

const char* vertexShader =
"#version 330 core \n"
"layout (location = 0) in vec3 aPos; \n"
"layout (location = 1) in vec2 aTexCoord; \n"

"out vec3 position; \n"
"out vec2 texCoord; \n"

"uniform ivec2 iResolution;"

"void main() { \n"
"    position = aPos; \n"
"    texCoord = aTexCoord * iResolution;\n"
"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); \n"
"}";

const char* errorFragmentShaderSource = " \
#version 330 core \n \
out vec4 FragColor; \n \
\
void main() { \n \
    FragColor = vec4(1, 0, 1, 1); \n \
}";

const char shaderHeader[] =
"#version 330 core\n"

"#define COLOR() \n"
"#define RANGE(a, b) \n"
"#define LIB(name) \n"

"out vec4 FragColor;\n"

"in vec3 position;\n"
"in vec2 texCoord;\n"

"uniform ivec2 iResolution;\n"
"uniform float iTime;\n"
"uniform float iTimeDelta;\n"
"uniform int   iFrame;\n"

"void mainImage(out vec4 fragColor, in vec2 fragCoord);\n"
"void main() {\n"
"   mainImage(FragColor, texCoord); \n"
"}\n\n"
;

ImVec2 TextureSizePresets[] = {
    ImVec2(500, 256),
    ImVec2(256, 256),
    ImVec2(512, 512),
    ImVec2(1024, 1024),
};

char* TextureSizeLabels[] = {
    "500 x 256",
    "256 x 256",
    "512 x 512",
    "1024 x 1024",
    "Custom...",
};

int selectedPresetIndex;

GLuint vertexShaderHandle;
GLuint errorShaderHandle;

double time;
double deltaTime;

int frame;

#define glUniform(size, type) glUniform ## size ## type ## v

template <typename T>
T Clamp(T v, T a, T b) { return (v < a) ? a : ((v > b) : b : v); }

template <typename T>
T Min(T v, T a) { return v < a ? v : a; }

template <typename T>
T Max(T v, T a) { return v > a ? v : a; }

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Str8 LoadShaderSource(Str8 filePath, MemoryArena* arena);

bool CompileShader(Shader* shader) {
    static const char* pathFormat = "shaders/lib/%.*s.glsl";

    int success;
    char infoLog[1024] = {};

    int shaderSourcesCount = shader->libsCount + 2;
    char** sources = (char**) PushArena(&temporaryArena, shaderSourcesCount * sizeof(char**));

    sources[0] = (char*) shaderHeader;

    for(int i = 0; i < shader->libsCount; i++) {
        Str8 path = {};
        path.length = IM_ARRAYSIZE(pathFormat) + shader->libs[i].name.length;
        path.string = (char*) PushArena(&temporaryArena, path.length);

        sprintf(path.string, pathFormat, shader->libs[i].name.length, shader->libs[i].name.string);

        Str8 source = LoadShaderSource(path, &temporaryArena);
        sources[i + 1] = source.string;
    }

    sources[shaderSourcesCount - 1] = shader->source.string;

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, shaderSourcesCount, sources, NULL);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragShader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Failed to compile fragment shader: %s \n", infoLog);
        fprintf(stderr, "\n %s \n\n", shader->source.string);

        return false;
    }

    GLuint linkedShader = glCreateProgram();
    glAttachShader(linkedShader, vertexShaderHandle);
    glAttachShader(linkedShader, fragShader);
    glLinkProgram(linkedShader);

    glGetShaderiv(linkedShader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(linkedShader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Failed to link shaders: %s \n", infoLog);

        return false;
    }

    shader->handle = linkedShader;
    return true;
}

Str8 LoadShaderSource(Str8 filePath, MemoryArena* arena) {
    Str8 ret = {};
    FILE* file = fopen(filePath.string, "rb");

    if(!file) {
        fprintf(stderr, "Can't open file");
        return ret;
    }

    fseek(file, 0, SEEK_END);
    long fileLength = ftell(file) + 1;
    fseek(file, 0, SEEK_SET);

    ret.length = fileLength;
    ret.string = (char*) PushArena(arena, fileLength);

    fread(ret.string, 1, fileLength, file);
    fclose(file);

    ret.string[fileLength] = '\0';

    return ret;
}

void CreateShader(Shader* shader) {
    shader->lastWriteTime = GetLastWriteTime(shader->filePath.string);

    shader->source   = LoadShaderSource(shader->filePath, &shader->shaderMemory);
    shader->uniforms = GetShaderUniforms(shader->source.string, &shader->shaderMemory, &shader->uniformsCount);
    shader->libs     = GetShaderLibs(shader->source.string, &shader->shaderMemory, &shader->libsCount);

    if(CompileShader(shader) == false) {
        shader->isValid = false;
        return;
    }

    shader->resolutionLocation = glGetUniformLocation(shader->handle, "iResolution");
    shader->timeLocation = glGetUniformLocation(shader->handle, "iTime");
    shader->timeDeltaLocation = glGetUniformLocation(shader->handle, "iTimeDelta");
    shader->frameLocation = glGetUniformLocation(shader->handle, "iFrame");

    for(int i = 0; i < shader->uniformsCount; i++) {
        shader->uniforms[i].location = glGetUniformLocation(shader->handle, shader->uniforms[i].name);
    }

    shader->isValid = true;
}

Shader CreateShaderFromFile(char* filePath) {
    Shader ret = {};
    ret.shaderMemory = CreateArena();

    ret.filePath.length = strlen(filePath) + 1;
    ret.filePath.string = (char*) PushArena(&ret.shaderMemory, ret.filePath.length);

    ret.lastWriteTime = GetLastWriteTime(filePath);

    memcpy(ret.filePath.string, filePath, ret.filePath.length);

    CreateShader(&ret);

    return ret;
}

void UnloadShader(Shader* shader) {
    glDeleteShader(shader->handle);

    ClearArena(&shader->shaderMemory);

    shader->isValid = false;
}

void ReloadShader(Shader* shader) {
    uint64_t pathLen = shader->filePath.length;
    char* temp = (char*) PushArena(&temporaryArena, pathLen);
    memcpy(temp, shader->filePath.string, pathLen);

    ClearArena(&shader->shaderMemory);
    shader->filePath.string = (char*) PushArena(&shader->shaderMemory, pathLen);
    memcpy(shader->filePath.string, temp, pathLen);

    glDeleteShader(shader->handle);
    
    CreateShader(shader);
}

Framebuffer CreateFramebuffer(int width, int height) {
    Framebuffer ret = {};

    ret.width = width;
    ret.height = height;

    glGenFramebuffers(1, &ret.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, ret.handle);

    glGenTextures(1, &ret.colorTexture);
    glBindTexture(GL_TEXTURE_2D, ret.colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ret.colorTexture, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Cannot create framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return ret;
}

void ResizeFrambuffer(Framebuffer* frambuffer, int width, int height) {
    assert(width > 1 && height > 1);

    if(frambuffer->width != width || frambuffer->height != height) {
        glBindTexture(GL_TEXTURE_2D, frambuffer->colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        frambuffer->width = width;
        frambuffer->height = height;
    }
}


void DrawMenuBar() {
    if(ImGui::BeginMainMenuBar()) {
        
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("Show/Hide ImGui help")) {
                show_demo_window = !show_demo_window;
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void DrawFloat(ShaderUniformData* uniform) {
    assert(uniform->type == UniformType_Float);

    bool changed = false;
    if(uniform->attributeFlags & UniformAttribFlag_Range) {
        changed = ImGui::SliderScalarN(uniform->name, ImGuiDataType_Float, uniform->floatValue, uniform->vectorLength, &uniform->minRangeValue, &uniform->maxRangeValue);
    }
    else if(uniform->attributeFlags & UniformAttribFlag_Color && 
        (uniform->vectorLength == 3 || uniform->vectorLength == 4))
    {
        if(uniform->vectorLength == 3) {
            changed = ImGui::ColorEdit3(uniform->name, uniform->floatValue);
        }
        else {
            changed = ImGui::ColorEdit4(uniform->name, uniform->floatValue);
        }
    }
    else {
        changed = ImGui::InputScalarN(uniform->name, ImGuiDataType_Float, uniform->floatValue, uniform->vectorLength);
    }

    if(changed) {
        switch(uniform->vectorLength) {
            case 1: glUniform1fv(uniform->location, 1, uniform->floatValue); break;
            case 2: glUniform2fv(uniform->location, 1, uniform->floatValue); break;
            case 3: glUniform3fv(uniform->location, 1, uniform->floatValue); break;
            case 4: glUniform4fv(uniform->location, 1, uniform->floatValue); break;
        }
    }
}

void DrawDouble(ShaderUniformData* uniform) {
    assert(uniform->type == UniformType_Double);

    bool changed = false;
    if(uniform->attributeFlags & UniformAttribFlag_Range) {
        double min = (double) uniform->minRangeValue;
        double max = (double) uniform->maxRangeValue;

        changed = ImGui::SliderScalarN(uniform->name, ImGuiDataType_Double, uniform->doubleValue, uniform->vectorLength, &min, &max);
    }
    else {
        changed = ImGui::InputScalarN(uniform->name, ImGuiDataType_Double, uniform->doubleValue, uniform->vectorLength);
    }


    if(changed) {
        switch(uniform->vectorLength) {
            case 1: glUniform1dv(uniform->location, 1, uniform->doubleValue); break;
            case 2: glUniform2dv(uniform->location, 1, uniform->doubleValue); break;
            case 3: glUniform3dv(uniform->location, 1, uniform->doubleValue); break;
            case 4: glUniform4dv(uniform->location, 1, uniform->doubleValue); break;
        }
    }
}

void DrawInt(ShaderUniformData* uniform) {
    assert(uniform->type == UniformType_Int);

    bool changed = false;
    if(uniform->attributeFlags & UniformAttribFlag_Range) {
        int min = (int) uniform->minRangeValue;
        int max = (int) uniform->maxRangeValue;

        changed = ImGui::SliderScalarN(uniform->name, ImGuiDataType_S32, &uniform->intValue, uniform->vectorLength, &min, &max);
    }
    else {
        changed = ImGui::InputScalarN(uniform->name, ImGuiDataType_S32, &uniform->intValue, uniform->vectorLength);
    }

    if(changed) {
        switch(uniform->vectorLength) {
            case 1: glUniform1iv(uniform->location, 1, uniform->intValue); break;
            case 2: glUniform2iv(uniform->location, 1, uniform->intValue); break;
            case 3: glUniform3iv(uniform->location, 1, uniform->intValue); break;
            case 4: glUniform4iv(uniform->location, 1, uniform->intValue); break;
        }
    }
}

void DrawUInt(ShaderUniformData* uniform) {
    assert(uniform->type == UniformType_UInt);

    bool changed = false;
    if(uniform->attributeFlags & UniformAttribFlag_Range) {
        uint32_t min = (uint32_t) Max(0.f, uniform->minRangeValue);
        uint32_t max = (uint32_t) Max(0.f, uniform->maxRangeValue);

        changed = ImGui::SliderScalarN(uniform->name, ImGuiDataType_U32, &uniform->uintValue, uniform->vectorLength, &min, &max);
    }
    else {
        changed = ImGui::InputScalarN(uniform->name, ImGuiDataType_U32, &uniform->uintValue, uniform->vectorLength);
    }

    if(changed) {
        switch(uniform->vectorLength) {
            case 1: glUniform1uiv(uniform->location, 1, uniform->uintValue); break;
            case 2: glUniform2uiv(uniform->location, 1, uniform->uintValue); break;
            case 3: glUniform3uiv(uniform->location, 1, uniform->uintValue); break;
            case 4: glUniform4uiv(uniform->location, 1, uniform->uintValue); break;
        }
    }

}

int main()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "TGFPA", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    if (gladLoadGL() == 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    temporaryArena = CreateArena();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    float vertices[] = { // Pos, UV
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, // bot left
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // bot right
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // top right
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, // top left
    };

    int indices[] = { 
        0, 1, 2,
        0, 2, 3
    };

    // Quad mesh
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    unsigned int EBO;
    glGenBuffers(1, &EBO);

    unsigned int VAO;
    glGenBuffers(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // built in shaders
    vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderHandle, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderHandle);

    int success;
    char infoLog[512];

    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShaderHandle, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Failed to compile BUILT-IN VERTEX shader: %s \n", infoLog);
    }

    GLuint errorFragmenHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(errorFragmenHandle, 1, &errorFragmentShaderSource, NULL);
    glCompileShader(errorFragmenHandle);

    glGetShaderiv(errorFragmenHandle, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(errorFragmenHandle, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Failed to compile ERROR FRAGMENT shader: %s \n", infoLog);
    }

    errorShaderHandle = glCreateProgram();
    glAttachShader(errorShaderHandle, vertexShaderHandle);
    glAttachShader(errorShaderHandle, errorFragmenHandle);
    glLinkProgram(errorShaderHandle);

    glGetShaderiv(errorShaderHandle, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(errorShaderHandle, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Failed to LINK ERROR shader: %s \n", infoLog);
    }

    glDeleteShader(errorFragmenHandle);

    Shader shader = CreateShaderFromFile("./shaders/test.glsl");
    Framebuffer framebuffer = CreateFramebuffer((int) TextureSizePresets[0].x, (int) TextureSizePresets[0].y);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // @Win32 Call
        FILETIME currentModifyTime = GetLastWriteTime(shader.filePath.string);
        if(CompareFileTime(&currentModifyTime, &shader.lastWriteTime) != 0) {
            ReloadShader(&shader);
            printf("RELOADING SHADER on path: %s \n", shader.filePath.string);
        }

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        deltaTime = glfwGetTime() - time;
        time = glfwGetTime();
        frame++;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        ImGui::SetNextWindowDockID(dockspaceID , ImGuiCond_Once);

        ImGui::Begin("Main");
        
        ImGui::BeginChild("Texture", ImVec2(512, 0));

        int presetsCount = IM_ARRAYSIZE(TextureSizePresets);
        if(ImGui::BeginCombo("Size", TextureSizeLabels[selectedPresetIndex])) {

            // NOTE: Labels should have (presetsCount + 1) elements
            for(int i = 0; i <= presetsCount; i++) {
                if(ImGui::Selectable(TextureSizeLabels[i], selectedPresetIndex == i)) {
                    selectedPresetIndex = i;

                    if(i < presetsCount) {
                        ResizeFrambuffer(&framebuffer, (int) TextureSizePresets[i].x, (int) TextureSizePresets[i].y);
                    }
                }
            }

            ImGui::EndCombo();
        }

        // NOTE: TODO: Move to other struct when (if?) multiple shaders are added
        static int width = 512;
        static int height = 512;
        if(selectedPresetIndex == presetsCount) {
            ImGui::PushItemWidth(ImGui::GetFontSize() * 12);

            ImGui::InputInt("Width", &width);
            ImGui::SameLine();
            ImGui::InputInt("Height", &height);
            ImGui::PopItemWidth();

            if(width <= 0) width = 1;
            if(height <= 0) height = 1;

            ImGui::SameLine();
            if(ImGui::Button("Apply")) {
                ResizeFrambuffer(&framebuffer, width, height);
            }
        }

        ImGui::Image((void*)(intptr_t)framebuffer.colorTexture, ImVec2((float) framebuffer.width, (float) framebuffer.height));
        if(ImGui::Button("Save")) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.handle);

            void* textureBuffer = PushArena(&temporaryArena, framebuffer.width * framebuffer.height);
            
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, framebuffer.width, framebuffer.height, GL_RGB, GL_UNSIGNED_BYTE, textureBuffer);
            stbi_write_png("test.png", framebuffer.width, framebuffer.height, 3 /*RGB*/, textureBuffer, framebuffer.width * 3);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ImGui::SameLine();
        if(ImGui::Button("Open")) {
            char* path = OpenFileDialog(&temporaryArena);
            if(path) {
                UnloadShader(&shader);

                int len = (int) strlen(path) + 1;
                shader.filePath.string = (char*) PushArena(&shader.shaderMemory, len);
                shader.filePath.length = len;

                memcpy(shader.filePath.string, path, len);

                ReloadShader(&shader);
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Uniforms", ImVec2(0, 0), true);

        static bool showUnsusedUniforms = true;
        ImGui::Checkbox("Show Unsused Uniforms", &showUnsusedUniforms);

        ImGui::Separator();

        for(int i = 0; i < shader.uniformsCount; i++) {
            if(showUnsusedUniforms == false && shader.uniforms[i].location == -1)
                continue;

            ShaderUniformData* uniform = shader.uniforms + i;

            switch(uniform->type) {
                case UniformType_Bool: {} break;

                case UniformType_UInt:   DrawUInt(uniform);   break;
                case UniformType_Int:    DrawInt(uniform);    break;
                case UniformType_Float:  DrawFloat(uniform);  break;
                case UniformType_Double: DrawDouble(uniform); break;
            }
        }
        ImGui::EndChild();

        ImGui::End();

        // Rendering

        ImGui::Render();

        int display_w, display_h;

        // Draw to framebuffer

        glUseProgram(shader.isValid ? 
                     shader.handle :
                     errorShaderHandle);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.handle);

        glUniform2i(shader.resolutionLocation, (int) framebuffer.width, (int) framebuffer.height);
        glUniform1f(shader.timeLocation, (float) time);
        glUniform1f(shader.timeDeltaLocation, (float) deltaTime);
        glUniform1i(shader.frameLocation, frame);

        glViewport(0, 0, framebuffer.width, framebuffer.height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // draw application
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);

        ClearArena(&temporaryArena);
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
