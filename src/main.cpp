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
#include <cmath>
#include <iostream>

int WINDOW_W = 1280;
int WINDOW_H = 720;
int GRID = 3;

const float SNAP_BASE = 0.09f;
const float SNAP_FACTOR = 1.6f;

struct PuzzlePiece {
    float x, y;
    float size;
    float u0, v0;
    float u1, v1;
    float tx, ty;
    bool snapped;
};

std::vector<PuzzlePiece> pieces;
GLuint tex = 0;
GLuint vao = 0, vbo = 0, ebo = 0;
GLuint texShader = 0;

bool prevMouseDown = false;
bool mouseDown = false;
int dragged = -1;
float grabOffsetX = 0.0f;
float grabOffsetY = 0.0f;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    WINDOW_W = width;
    WINDOW_H = height;
    glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

GLuint compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, NULL, log);
        fprintf(stderr, "shader error: %s\n", log);
    }
    return s;
}

GLuint makeProgram(const char* vs, const char* fs)
{
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, NULL, log);
        fprintf(stderr, "program link error: %s\n", log);
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

GLuint loadTexture(const char* path, int& w, int& h)
{
    int ch;
    stbi_set_flip_vertically_on_load(0);

    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) {
        fprintf(stderr, "Failed to load: %s (%s)\n", path, stbi_failure_reason());
        return 0;
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return t;
}

std::vector<PuzzlePiece> generatePieces(int grid)
{
    srand((unsigned)time(NULL));
    std::vector<PuzzlePiece> out;
    out.reserve(grid * grid);

    float half = 0.5f / grid;
    float cell = 2.0f * half;

    for (int row = 0; row < grid; ++row) {
        for (int col = 0; col < grid; ++col) {
            PuzzlePiece p;

			stbi_set_flip_vertically_on_load(1);
            float u_left  = col / float(grid);
            float u_right = (col + 1) / float(grid);
            float v_top   = row / float(grid);
            float v_bottom= (row + 1) / float(grid);

            p.u0 = u_left;
            p.u1 = u_right;
            p.v0 = v_bottom;
            p.v1 = v_top;

            p.size = half;

            p.tx = ((col + 0.5f) - grid / 2.0f) * cell;
            p.ty = ((row + 0.5f) - grid / 2.0f) * cell;

            p.x = ((rand() % 2000) / 1000.0f - 1.0f) * 0.85f;
            p.y = ((rand() % 2000) / 1000.0f - 1.0f) * 0.85f;

            p.snapped = false;
            out.push_back(p);
        }
    }
    return out;
}

int bringFront(int idx)
{
    if (idx < 0 || idx >= (int)pieces.size()) return idx;
    PuzzlePiece p = pieces[idx];
    pieces.erase(pieces.begin() + idx);
    pieces.push_back(p);
    return (int)pieces.size() - 1;
}

int main()
{
#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, "jigsaw", NULL, NULL);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to init GLAD\n");
        return -1;
    }

    printf("OpenGL: %s\n", glGetString(GL_VERSION));

    const char* filters[] = {"*.jpg", "*.png"};
    const char* chosen = tinyfd_openFileDialog("Choose image for puzzle", "", 2, filters, NULL, 0);
    if (!chosen) return 0;

    int imgW = 0, imgH = 0;
    tex = loadTexture(chosen, imgW, imgH);
    if (!tex) return 0;

    pieces = generatePieces(GRID);

    float quad[16] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
		0.5f, -0.5f, 1.0f, 0.0f,
		0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f
    };
    unsigned int idx[] = {0,1,2, 2,3,0};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));

    const char* vs =
        "#version 410 core\n"
        "layout(location=0) in vec2 pos;\n"
        "layout(location=1) in vec2 uv;\n"
        "out vec2 v_uv;\n"
        "void main(){ v_uv = uv; gl_Position = vec4(pos,0,1); }\n";

    const char* fs =
        "#version 410 core\n"
        "in vec2 v_uv;\n"
        "out vec4 frag;\n"
        "uniform sampler2D tex0;\n"
        "void main(){ frag = texture(tex0, v_uv); }\n";

    texShader = makeProgram(vs, fs);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        mouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        float xN = (float)((mx / WINDOW_W) * 2.0 - 1.0);
        float yN = (float)(1.0 - (my / WINDOW_H) * 2.0);

        if (mouseDown && !prevMouseDown) {
            for (int i = (int)pieces.size() - 1; i >= 0; --i) {
                PuzzlePiece &p = pieces[i];
                if (p.snapped) continue;
                if (xN > p.x - p.size && xN < p.x + p.size &&
                    yN > p.y - p.size && yN < p.y + p.size) {
                    dragged = bringFront(i);
                    grabOffsetX = xN - pieces[dragged].x;
                    grabOffsetY = yN - pieces[dragged].y;
                    break;
                }
            }
        }

        if (!mouseDown && prevMouseDown) {
            if (dragged != -1) {
                PuzzlePiece &p = pieces[dragged];
                float dx = p.x - p.tx;
                float dy = p.y - p.ty;
                float centerDist = sqrtf(dx*dx + dy*dy);

                float mx_d = xN - p.tx;
                float my_d = yN - p.ty;
                float mouseDist = sqrtf(mx_d*mx_d + my_d*my_d);

                float threshold = fmaxf(SNAP_BASE, p.size * SNAP_FACTOR);

                if (centerDist <= threshold || mouseDist <= threshold) {
                    p.x = p.tx;
                    p.y = p.ty;
                    p.snapped = true;
                }
            }
            dragged = -1;
        }

        if (mouseDown && dragged != -1) {
            pieces[dragged].x = xN - grabOffsetX;
            pieces[dragged].y = yN - grabOffsetY;
        }

        prevMouseDown = mouseDown;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(texShader);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        GLint texloc = glGetUniformLocation(texShader, "tex0");
        glUniform1i(texloc, 0);

        for (auto &p : pieces) {
            float verts[16] = {
                p.x - p.size, p.y - p.size,  p.u0, p.v0,
                p.x + p.size, p.y - p.size,  p.u1, p.v0,
                p.x + p.size, p.y + p.size,  p.u1, p.v1,
                p.x - p.size, p.y + p.size,  p.u0, p.v1
            };
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
    }

    if (tex) glDeleteTextures(1, &tex);
    glDeleteProgram(texShader);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
