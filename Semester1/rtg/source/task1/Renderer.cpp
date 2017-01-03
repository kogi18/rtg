#include "Renderer.h"
#include "iostream"

class GLException : public std::exception
{
private:
	GLint error_code;

public:
	GLException(GLint error) : error_code(error) {}
	virtual const char* what() const noexcept
	{
		switch (error_code)
		{
		case (GL_INVALID_ENUM):
			return "Enumeration parameter is not a legal enumeration for that function.";
		case (GL_INVALID_VALUE):
			return "Illegal parameter value for that function.";
		case (GL_INVALID_OPERATION):
			return "This operation cannot be executed in the current state of OpenGL.";
		case (GL_STACK_OVERFLOW):
			return "Stack overflow occurred.";
		case (GL_STACK_UNDERFLOW):
			return "Stack underflow occurred.";
		case (GL_OUT_OF_MEMORY):
			return "Out of memory.";
		case (GL_INVALID_FRAMEBUFFER_OPERATION):
			return "Operation could not be performed in the current state of the frame buffer.";
		default:
			return "Unknown error! Please check error code!";
		}
	}
};

#ifdef _DEBUG
#define GL_SAFE_CALL(A) ([&]                \
{                                           \
	struct error_guard						\
	{                                       \
		~error_guard() noexcept(false)      \
		{                                   \
			GLenum error = glGetError();    \
			if (error != GL_NO_ERROR)       \
			{                               \
				GLException ex(error);      \
				std::cerr << ex.what();     \
				throw ex;                   \
			}                               \
		}                                   \
	} guard;                                \
	return A;                               \
}())

class ShaderException : public std::exception {
private: std::string log;
public: ShaderException(std::string&& log) : log(std::move(log)) {}
		virtual const char* what() const noexcept { return log.c_str(); }
};

void checkShader(const GLint shader) {
	GLint isCompiled; GL_SAFE_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled));
	if (isCompiled != GL_TRUE) {
		GLint log_length; GL_SAFE_CALL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length));
		auto buffer = std::unique_ptr<char[]>(new char[log_length]); GL_SAFE_CALL(glGetShaderInfoLog(shader, log_length, &log_length, buffer.get()));
		ShaderException ex(std::string(buffer.get(), log_length - 1)); std::cerr << ex.what(); throw ex;
	}
}

#else
#define GL_SAFE_CALL(A) (A)
#endif

const char* vertex_shader_src = R"""(
#version 330

out vec4 vertex_color;

void main(){
	if(gl_VertexID < 1){
		gl_Position = vec4(-0.5f, -0.5f, 0.0f, 1.0f);
		vertex_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else{
		if(gl_VertexID < 2){
			gl_Position = vec4(0.0f, 0.5f, 0.0f, 1.0f);
			vertex_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		}
		else{
			gl_Position = vec4(0.5f, -0.5f, 0.0f, 1.0f);
			vertex_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
		}
	}
}
)""";
const char* fragment_shader_src = R"""(
#version 330

in vec4 vertex_color;
out vec4 fragment_color;

void main(){
	//fragment_color = vertex_color;
	fragment_color = sqrt(sqrt(vertex_color));
	fragment_color.a = 1.0f;
}
)""";


Renderer::Renderer(GL::platform::Window& window)
	: BasicRenderer(window,3,3)
{
	glClearColor(0.1f, 0.3f, 1.0f, 1.0f);

	window.attach(this);
}

void Renderer::resize(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, viewport_width, viewport_height);

	// the needed VAO declaration and binding
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create program to which we connect the shaders
	GLuint program = GL_SAFE_CALL(glCreateProgram());

	// load vertex shader
	GLuint vertexShader = GL_SAFE_CALL(glCreateShader(GL_VERTEX_SHADER));
	GL_SAFE_CALL(glShaderSource(vertexShader, 1, &vertex_shader_src, 0));
	GL_SAFE_CALL(glCompileShader(vertexShader));
	GL_SAFE_CALL(glAttachShader(program, vertexShader));

	// load fragment shader
	GLuint fragmentShader = GL_SAFE_CALL(glCreateShader(GL_FRAGMENT_SHADER));
	GL_SAFE_CALL(glShaderSource(fragmentShader, 1, &fragment_shader_src, 0));
	GL_SAFE_CALL(glCompileShader(fragmentShader));
	GL_SAFE_CALL(glAttachShader(program, fragmentShader));

	// link the program to the GPU
	GL_SAFE_CALL(glLinkProgram(program));

	// use the program
	glUseProgram(program);
	// start vertex shader to draw the triangle from the first 3 vertices
	GL_SAFE_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));

	swapBuffers();
}
