#define UNUSED __attribute__((unused))

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void framebuffer_size_callback(UNUSED GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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
    if (!window) {
        return EXIT_FAILURE;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	float points[] = {
		0.0f,  0.5f,  0.0f, // x,y,z of first point.
		0.5f, -0.5f,  0.0f, // x,y,z of second point.
		-0.5f, -0.5f,  0.0f  // x,y,z of third point.
	};

	GLuint vbo = 0;
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, 9 * sizeof( float ), points, GL_STATIC_DRAW );

	// Vertex/Fragment shader sample
	const char* vertex_shader =
		"#version 410 core\n"
		"in vec3 vp;"
		"void main() {"
		"  gl_Position = vec4( vp, 1.0 );"
		"}";

	// VAO (vertex array object), we need to save vertex data to then feed it to shaders
	const char* fragment_shader =
		"#version 410 core\n"
		"out vec4 frag_colour;"
		"void main() {"
		"  frag_colour = vec4( 0.5, 0.0, 0.5, 1.0 );"
		"}";

	GLuint vs = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vs, 1, &vertex_shader, NULL );
	glCompileShader( vs );
	GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fs, 1, &fragment_shader, NULL );
	glCompileShader( fs );

	GLuint shader_program = glCreateProgram();
	glAttachShader( shader_program, fs );
	glAttachShader( shader_program, vs );
	glLinkProgram( shader_program );
	
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glUseProgram( shader_program );
		glBindVertexArray( vbo );
		glDrawArrays( GL_TRIANGLES, 0, 3 );
		glfwSwapBuffers( window );
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
