


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

//GLfloat vertexList[] = {
//	0.0f, 0.0f,
//	-0.5f, -0.5f,
//	0.0f, 0.5f,
//	0.5f, -0.5f
//};

float sin60cos30HALVED = 0.866035403f*0.5f;

GLfloat vertexList[] = {
	// center of fan
	0.0f, 0.0f,
	// 1. quadrant
	0.5f, 0.0f,					// 0 degree of unit circle
	sin60cos30HALVED, 0.25f,	// 30 degree of unit circle
	0.25f, sin60cos30HALVED,	// 60 degree of unit circle
	0.0f, 0.5f,					// 90 degree of unit circle
	// 2. quadrant
	-0.25f, sin60cos30HALVED,	// 120 degree of unit circle
	-sin60cos30HALVED, 0.25f,	// 150 degree of unit circle
	-0.5f, 0.0f,				// 180 degree of unit circle
	// 3. quadrant
	-sin60cos30HALVED, -0.25f,	// 210 degree of unit circle
	-0.25f, -sin60cos30HALVED,	// 240 degree of unit circle
	0.0f, -0.5f,				// 270 degree of unit circle
	// 4. quadrant
	0.25f, -sin60cos30HALVED,	// 300 degree of unit circle
	sin60cos30HALVED, -0.25f,	// 330 degree of unit circle
	0.5f, 0.0f,					// 360 degree of unit circle - for correct interpolation
};

GLfloat colorList[] = {
	// center of fan
	1.0f, 1.0f, 1.0f,
	// 1. quadrant
	1.0f, 0.0f, 0.0f,
	0.75f, 0.25f, 0.0f,
	0.25f, 0.75f, 0.0f,
	0.0f, 1.0f, 0.0f,
	// 2. quadrant
	0.0f, 0.75f, 0.25f,
	0.0f, 0.25f, 0.75f,
	0.0f, 0.0f, 1.0f,
	// 3. quadrant
	0.25f, 0.0f, 1.0f,
	0.75f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	// 4. quadrant
	1.0f, 0.0f, 0.75f,
	1.0f, 0.0f, 0.25f,
	1.0f, 0.0f, 0.0f // has to be same as 1st in 1st quadrant
};


const char* vertex_shader_src = R"""(
#version 330

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

out vec4 vertex_color;

void main(){
	gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
	vertex_color = vec4(color.r, color.g, color.b, 1.0f);
}
)""";
const char* fragment_shader_src = R"""(
#version 330

in vec4 vertex_color;
out vec4 fragment_color;

void main(){
	fragment_color = vertex_color;
}
)""";

Renderer::Renderer(GL::platform::Window& window)
	: BasicRenderer(window, 3, 3)
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

	// the VAO declaration and binding
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create VOB to send vertex and color data
	GLuint vertexVOB, colorVOB;
	// request names, bind for the 1st time, bind the actual data
	glGenBuffers(1, &vertexVOB);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexList), vertexList, GL_STATIC_DRAW);
	
	glGenBuffers(1, &colorVOB);
	glBindBuffer(GL_ARRAY_BUFFER, colorVOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colorList), colorList, GL_STATIC_DRAW);

	//configzre VAO layout
	// set position at 0
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set color at 1
	glBindBuffer(GL_ARRAY_BUFFER, colorVOB);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// bind VAO
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
	// start vertex shader to draw the triangle fan
	GL_SAFE_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 14));

	swapBuffers();
}
