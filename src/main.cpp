#define UNUSED __attribute__((unused))
#define STB_IMAGE_IMPLEMENTATION
#define TINYFILEDIALOGS_IMPLEMENTATION

#include "stb_image.h"
#include "tinyfiledialogs.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

struct PuzzlePiece {
    float x, y;
    float size;
    float u0, v0;
    float u1, v1;
};

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static GLFWwindow* init(void)
{
    if (!glfwInit()) {
        fprintf(stderr, "GLFW initialization failed\n");
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Renderer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "GLFW window creation failed\n");
        glfwTerminate();
        return NULL;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return NULL;
    }

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));
    return window;
}

std::vector<PuzzlePiece> createPuzzlePieces(int grid)
{
    srand(time(NULL));

    std::vector<PuzzlePiece> pieces;
    pieces.reserve(grid * grid);

    for (int y = 0; y < grid; y++) {
        for (int x = 0; x < grid; x++) {

            float u0 = x / float(grid);
            float v0 = y / float(grid);
            float u1 = (x+1) / float(grid);
            float v1 = (y+1) / float(grid);

            float rx = ((rand() % 2000) / 1000.0f - 1.0f) * 0.8f;
            float ry = ((rand() % 2000) / 1000.0f - 1.0f) * 0.8f;

            PuzzlePiece p;
            p.x = rx;
            p.y = ry;
            p.size = 0.5f / grid;
            p.u0 = u0;
            p.v0 = v0;
            p.u1 = u1;
            p.v1 = v1;

            pieces.push_back(p);
        }
    }

    return pieces;
}

int main(int argc, char** argv)
{
    GLFWwindow* window = init();
    if (!window) return EXIT_FAILURE;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);

    unsigned char* data = stbi_load("kawai.jpg", &w, &h, &ch, 4);
    if (!data) {
        printf("Failed to load image\n");
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    std::vector<PuzzlePiece> pieces = createPuzzlePieces(3);

    GLuint vao, vbo, ebo;

    float quadVerts[] = {
        -0.5f, -0.5f, 0, 0,
         0.5f, -0.5f, 1, 0,
         0.5f,  0.5f, 1, 1,
        -0.5f,  0.5f, 0, 1
    };

    unsigned int indices[] = {0,1,2, 2,3,0};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    const char* vs_src =
        "#version 410 core\n"
        "layout(location = 0) in vec2 pos;\n"
        "layout(location = 1) in vec2 uv;\n"
        "out vec2 v_uv;\n"
        "void main(){ gl_Position = vec4(pos, 0.0, 1.0); v_uv = uv; }\n";

    const char* fs_src =
        "#version 410 core\n"
        "in vec2 v_uv;\n"
        "out vec4 frag;\n"
        "uniform sampler2D tex0;\n"
        "void main(){ frag = texture(tex0, v_uv); }\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, NULL);
    glCompileShader(fs);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);

    glDeleteShader(vs);
    glDeleteShader(fs);

    while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, tex);

        for (auto& p : pieces)
        {
            float s = p.size;

            float verts[] = {
                p.x - s, p.y - s,  p.u0, p.v1,
                p.x + s, p.y - s,  p.u1, p.v1,
                p.x + s, p.y + s,  p.u1, p.v0,
                p.x - s, p.y + s,  p.u0, p.v0
            };

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
