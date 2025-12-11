#define UNUSED __attribute__((unused))
#define STB_IMAGE_IMPLEMENTATION
#define TINYFILEDIALOGS_IMPLEMENTATION

#include "stb_image.h"
#include "tinyfiledialogs.h"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <ctime>
#include <glad/glad.h>
#include <iostream>
#include <vector>

struct PuzzlePiece
{
  float x, y;
  float size;
  float u0, v0;
  float u1, v1;
};

bool mouseDown = false;
int draggedPiece = -1;
float grabOffsetX = 0;
float grabOffsetY = 0;

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height)
{
  glViewport(0, 0, width, height);
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
      mouseDown = true;
    if (action == GLFW_RELEASE)
    {
      mouseDown = false;
      draggedPiece = -1;
    }
  }
}

static GLFWwindow *init(void)
{
  if (!glfwInit())
    return NULL;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(1280, 720, "Renderer", NULL, NULL);
  if (!window)
    return NULL;

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  return window;
}

std::vector<PuzzlePiece> createPuzzlePieces(int grid)
{
  srand(time(NULL));
  std::vector<PuzzlePiece> pieces;
  pieces.reserve(grid * grid);

  for (int y = 0; y < grid; y++)
  {
    for (int x = 0; x < grid; x++)
    {
      float u0 = x / float(grid);
      float v0 = y / float(grid);
      float u1 = (x + 1) / float(grid);
      float v1 = (y + 1) / float(grid);

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

GLuint loadTexture(const char *file, int &w, int &h)
{
  int ch;
  unsigned char *data = stbi_load(file, &w, &h, &ch, 4);
  if (!data)
  {
    printf("Failed to load: %s\n", file);
    return 0;
  }

  GLuint t;
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);
  return t;
}

int main()
{
  GLFWwindow *window = init();
  if (!window)
    return EXIT_FAILURE;

  const char *filters[] = {"*.jpg", "*.png"};
  const char *file =
      tinyfd_openFileDialog("Choose an image", "", 2, filters, NULL, 0);

  if (!file)
    return 0;

  int w, h;
  GLuint tex = loadTexture(file, w, h);
  if (!tex)
    return 0;

  std::vector<PuzzlePiece> pieces = createPuzzlePieces(3);

  GLuint vao, vbo, ebo;

  float quadVerts[16] = {0};
  unsigned int indices[] = {0, 1, 2, 2, 3, 0};

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));

  const char *vs_src =
      "#version 410 core\n"
      "layout(location = 0) in vec2 pos;\n"
      "layout(location = 1) in vec2 uv;\n"
      "out vec2 v_uv;\n"
      "void main(){ gl_Position = vec4(pos, 0.0, 1.0); v_uv = uv; }\n";

  const char *fs_src = "#version 410 core\n"
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

  while (!glfwWindowShouldClose(window))
  {
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float xNdc = (mx / 1280.0f) * 2.0f - 1.0f;
    float yNdc = 1.0f - (my / 720.0f) * 2.0f;

    if (mouseDown && draggedPiece == -1)
    {
      for (int i = pieces.size() - 1; i >= 0; i--)
      {
        PuzzlePiece &p = pieces[i];
        float s = p.size;

        if (xNdc > p.x - s && xNdc < p.x + s && yNdc > p.y - s &&
            yNdc < p.y + s)
        {
          draggedPiece = i;
          grabOffsetX = xNdc - p.x;
          grabOffsetY = yNdc - p.y;
          break;
        }
      }
    }

    if (mouseDown && draggedPiece != -1)
    {
      pieces[draggedPiece].x = xNdc - grabOffsetX;
      pieces[draggedPiece].y = yNdc - grabOffsetY;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, tex);

    for (PuzzlePiece &p : pieces)
    {
      float s = p.size;
      float verts[] = {p.x - s, p.y - s, p.u0, p.v1, p.x + s, p.y - s,
                       p.u1, p.v1, p.x + s, p.y + s, p.u1, p.v0,
                       p.x - s, p.y + s, p.u0, p.v0};

      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return 0;
}