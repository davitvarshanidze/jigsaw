#define UNUSED __attribute__((unused))
#define STB_IMAGE_IMPLEMENTATION
#define TINYFILEDIALOGS_IMPLEMENTATION
#include "stb_image.h"
#include "tinyfiledialogs.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void framebuffer_size_callback(UNUSED GLFWwindow* window, int width, int height)
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
    glfwSetKeyCallback(window, key_callback);
    if (!window) {
        fprintf(stderr, "GLFW window creation failed\n");
        glfwTerminate();
        return NULL;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return NULL;
    }

    const char *version = (const char *)glGetString(GL_VERSION);
    printf("OpenGL version: %s\n", version);

    return window;
}

int main(UNUSED int argc, UNUSED char **argv)
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

    float quad[] = {
        -0.5f, -0.5f,  0.0f, 0.0f,
		0.5f, -0.5f,  1.0f, 0.0f,
		0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };

    unsigned int idx[] = {0,1,2, 2,3,0};

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    const char* vs_src =
        "#version 410 core\n"
        "layout(location = 0) in vec2 pos;\n"
        "layout(location = 1) in vec2 uv;\n"
        "out vec2 v_uv;\n"
        "void main(){\n"
        "    gl_Position = vec4(pos, 0.0, 1.0);\n"
        "    v_uv = uv;\n"
        "}\n";

    const char* fs_src =
        "#version 410 core\n"
        "in vec2 v_uv;\n"
        "out vec4 frag;\n"
        "uniform sampler2D tex0;\n"
        "void main(){\n"
        "    frag = texture(tex0, v_uv);\n"
        "}\n";

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

	struct Button {
		float x0, y0, x1, y1;
	};
	
	Button chooseButton = {-0.3f, -0.9f, 0.3f, -0.7f};

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float xNDC = (mouseX / 1280) * 2.0f - 1.0f;
	float yNDC = 1.0f - (mouseY / 720) * 2.0f;

	if (xNDC >= chooseButton.x0 && xNDC <= chooseButton.x1 &&
		yNDC >= chooseButton.y0 && yNDC <= chooseButton.y1) {
	}

	const char *filterPatterns[] = {"*.jpg","*.png"};
	const char *file = tinyfd_openFileDialog(
		"Choose an image",
		"",
		2,
		filterPatterns,
		NULL,
		0
		);

	if (file) {
		glDeleteTextures(1, &tex);

		int w,h,ch;
		unsigned char* data = stbi_load(file, &w, &h, &ch, 4);
		if (data) {
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			stbi_image_free(data);
		}
	}

	// @main-loop
    while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, tex);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
