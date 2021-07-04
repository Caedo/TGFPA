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

#include "memory_management.h"

#include "platform_win32.cpp"
#include "memory_management.cpp"

MemoryArena temporaryArena;

struct Shader {
    MemoryArena shaderMemory;

    char* source;
    char* filePath;

    bool isValid;

    GLuint handle;

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

const char* vertexShader = " \
#version 330 core \n\
layout (location = 0) in vec3 aPos; \n\
\
out vec3 position; \n \
\
void main() { \n\
    position = aPos; \n \
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); \n\
}";

const char* errorFragmentShaderSource = " \
#version 330 core \n \
out vec4 FragColor; \n \
\
void main() { \n \
    FragColor = vec4(1, 0, 1, 1); \n \
}";

const char shaderHeader[] = " \
#version 330 core \n \
out vec4 FragColor; \n \
in vec3 position; \n \
";

GLuint vertexShaderHandle;
GLuint errorShaderHandle;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool CompileShader(const char* fragmentSource, GLuint* shader) {
    int success;
    char infoLog[1024];

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragmentSource, NULL);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragShader, sizeof(infoLog), NULL, infoLog);
        // fprintf(stderr, "Failed to compile fragment shader: %s \n", infoLog);
        // fprintf(stderr, "\n %s \n\n", fragmentSource);

        return false;
    }

    GLuint linkedShader = glCreateProgram();
    glAttachShader(linkedShader, vertexShaderHandle);
    glAttachShader(linkedShader, fragShader);
    glLinkProgram(linkedShader);

    glGetShaderiv(linkedShader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(linkedShader, sizeof(infoLog), NULL, infoLog);
        // fprintf(stderr, "Failed to link shaders: %s \n", infoLog);

        return false;
    }

    *shader = linkedShader;
    return true;
}

Shader LoadShader(char* filePath) {
    Shader ret = {};
    ret.shaderMemory = CreateArena();

    ret.filePath = (char*) PushArena(&ret.shaderMemory, strlen(filePath) + 1);
    memcpy(ret.filePath, filePath, strlen(filePath) + 1);

    FILE* file = fopen(filePath, "rb");

    if(!file) {
        fprintf(stderr, "Can't open file");
        return ret;
    }

    ret.lastWriteTime = GetLastWriteTime(filePath);

    fseek(file, 0, SEEK_END);
    long fileLength = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    int headerLength = IM_ARRAYSIZE(shaderHeader) - 1;
    int sourceLength = (headerLength + fileLength) + 1;

    ret.source = (char*) PushArena(&ret.shaderMemory, sourceLength);

    strcpy(ret.source, shaderHeader);
    fread(ret.source + headerLength, 1, fileLength, file);

    ret.source[headerLength + fileLength] = '\0';

    if(CompileShader(ret.source, &ret.handle) == false) {
        return ret;
    }

    fclose(file);

    ret.isValid = true;
    return ret;
}

void UnloadShader(Shader* shader) {
    glDeleteShader(shader->handle);

    DestroyArena(&shader->shaderMemory);

    shader->isValid = false;
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

    float vertices[] = {
        -1.0f, -1.0f, 0.0f, // bot left
         1.0f, -1.0f, 0.0f, // bot right
         1.0f,  1.0f, 0.0f, // top right
        -1.0f,  1.0f, 0.0f // top left
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

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

    Shader shader = LoadShader("./shaders/test.glsl");
    Framebuffer framebuffer = CreateFramebuffer(512, 512);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // @Win32 Call
        FILETIME currentModifyTime = GetLastWriteTime(shader.filePath);
        if(CompareFileTime(&currentModifyTime, &shader.lastWriteTime) != 0) {
            UnloadShader(&shader);
            shader = LoadShader("./shaders/test.glsl");

            printf("RELOADING SHADER on path: %s \n", shader.filePath);
        }

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Image((void*)(intptr_t)framebuffer.colorTexture, ImVec2(512, 512));
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
            printf(path);
        }

        // Rendering

        ImGui::Render();

        int display_w, display_h;

        // Draw to framebuffer

        glUseProgram(shader.isValid ? 
                     shader.handle :
                     errorShaderHandle);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.handle);

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
