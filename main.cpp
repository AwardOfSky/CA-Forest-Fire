// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

#define RES_X 1920
#define RES_Y 1000

// time variables
unsigned int frameCount = 0;
unsigned int fps = 0;
clock_t lastTime;

const char *window_title = "Forest Fire";

static unsigned int compile_shader(unsigned int type, const char *string);
static unsigned int create_shader(const char *vertex_shader, const char *fragment_shader);
char *file_to_string(const char *filename);
void timer(void);

int main(void) {
	// Resolution
	int windowWidth = RES_X;
	int windowHeight = RES_Y;

	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	// Open a window and create its OpenGL context (compatibility up until OpenGL 3.3)
	window = glfwCreateWindow(RES_X, RES_Y, window_title, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		glfwTerminate();
		getchar();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
	// Set window position on success
	glfwSetWindowPos(window, 0, 28);

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	
	// Clear color buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// Enable depth test
	glDisable(GL_DEPTH_TEST);
	// Cull faces not facing the camera
	glEnable(GL_CULL_FACE);

	// Create and compile our GLSL program from the shaders
	GLuint programID = create_shader(file_to_string("vertex.shader"), file_to_string("fragment.shader"));
	// Set uniform variable locations from the start as we are not re-linking the program
	GLuint TextureID = glGetUniformLocation(programID, "render_tex_pass");
	GLuint time_loc = glGetUniformLocation(programID, "time");
	GLuint resolution_loc = glGetUniformLocation(programID, "resolution");
	GLuint frame_count_loc = glGetUniformLocation(programID, "frame_count");
	glUseProgram(programID);

	// Render to Texture
	int err;
	//create frame buffer
	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	// Create one texture
	GLuint Texture;
	glGenTextures(1, &Texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	if ((err = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Error creating tex0 %x\n", err);
	}
	// Create another texture
	GLuint renderedTexture;
	glGenTextures(1, &renderedTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderedTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	if ((err = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Error Error creating tex1 %x\n", err);
	}

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return false;
	}

	// The fullscreen quad's FBO
	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
	};
	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile the passer shader that outputs the new texture to the screen
	GLuint quad_programID = create_shader(file_to_string("vertex_passt.shader"), file_to_string("fragment_passt.shader"));
	GLuint texID = glGetUniformLocation(quad_programID, "render_tex_pass");

	do {
		// Render to Texture
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		if (!(frameCount & 1)) {
			glDrawBuffer(GL_COLOR_ATTACHMENT1);
		} else {
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
		glViewport(0, 0, windowWidth, windowHeight);
		glUseProgram(programID); {

			glActiveTexture(GL_TEXTURE0);
			if (!(frameCount & 1)) {
				glBindTexture(GL_TEXTURE_2D, Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, renderedTexture);
			}

			glUniform1i(TextureID, 0);
			glUniform2f(resolution_loc, (float)RES_X, (float)RES_Y);
			glUniform1ui(frame_count_loc, frameCount);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);
			// Draw the triangles !
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
			glDisableVertexAttribArray(0);
		} glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);


		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0); 		//glDrawBuffer(GL_BACK);
		// Render on the whole framebuffer, complete from the lower left corner to the upper right
		glViewport(0, 0, windowWidth, windowHeight);
		// Clear the screen // glClear(GL_COLOR_BUFFER_BIT);
		// Use our shader
		glUseProgram(quad_programID); {

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			if (!(frameCount & 1)) {
				glBindTexture(GL_TEXTURE_2D, renderedTexture);
			} else {
				glBindTexture(GL_TEXTURE_2D, Texture);
			}

			// Set our "renderedTexture" sampler to use Texture Unit 0
			glUniform1i(texID, 0);
			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);
			// Draw the triangles !
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
			glDisableVertexAttribArray(0);
		} glUseProgram(0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		timer();
		++frameCount;

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader //glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(programID);
	glDeleteProgram(quad_programID);
	glDeleteTextures(1, &Texture);
	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteTextures(1, &renderedTexture);
	glDeleteBuffers(1, &quad_vertexbuffer);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

static unsigned int compile_shader(unsigned int type, const char *string) {
	unsigned int id = glCreateShader(type);
	const char *src = string;
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char *message = (char *)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		printf("Failed to compile %s shader!\n",
			(type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
		printf("%s\n", message);
		glDeleteShader(id);
		id = 0;
	}

	return id;	// TODO: Error handling
}

static unsigned int create_shader(const char *vertex_shader, const char *fragment_shader) {
	unsigned int program = glCreateProgram();
	unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
	unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

char *file_to_string(const char *filename) {
	char *buffer = NULL;
	unsigned int string_size;
	FILE *f_handler = fopen(filename, "rb"); // needs to be read in binary
	if (f_handler) {							 // because windows is retarded
		fseek(f_handler, 0, SEEK_END); // point to last byte of file
		string_size = ftell(f_handler); // size of current pos from start in bytes (file size)
		rewind(f_handler); // go back to the start of the file
		buffer = (char *)malloc((string_size + 1) * sizeof(char)); // +1 cuz null terminator
		fread(buffer, sizeof(char), string_size, f_handler); // read file in 1 go
		buffer[string_size] = '\0'; // null terminate it
		fclose(f_handler); // close file
	}
	else {
		printf("Error in 'file_to_string' function: Could not open file: '%s'\n", filename);
	}
	return buffer;
}

void timer(void) {
	++fps;
	if (clock() - lastTime > CLOCKS_PER_SEC) {
		lastTime = clock();
		char title[64];
		sprintf(title, "%s - FPS: %d", window_title, fps);
		glfwSetWindowTitle(window, title);
		fps = 0;
	}
}