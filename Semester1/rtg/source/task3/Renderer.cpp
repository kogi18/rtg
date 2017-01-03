
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

// 8 triangles * 3 homogenous verteces = 24 verteces (72 floats)
GLfloat vertexList[] = {
	// TOP
	-1.0f, 0.0f, 1.0f,	1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	// Front triangle
	1.0f, 0.0f, 1.0f,	1.0f, 0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	// Right triangle
	1.0f, 0.0f, -1.0f,	-1.0f, 0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	// Back triangle
	-1.0f, 0.0f, -1.0f,	-1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	// Left triangle
	// BOTTOM
	-1.0f, 0.0f, 1.0f,	0.0f, -1.0f, 0.0f,	1.0f, 0.0f, 1.0f,	// Front triangle
	1.0f, 0.0f, 1.0f,	0.0f, -1.0f, 0.0f,	1.0f, 0.0f, -1.0f,	// Right triangle
	1.0f, 0.0f, -1.0f,	0.0f, -1.0f, 0.0f,	-1.0f, 0.0f, -1.0f,	// Back triangle
	-1.0f, 0.0f, -1.0f,	0.0f, -1.0f, 0.0f,	-1.0f, 0.0f, 1.0f	// Left triangle
};

// define the indexes for same verteces
int sameVertexindeces[] = {
	2, 5, 8, 11,	// TOP (0, 1, 0)
	0, 10, 12, 23,	// FRONT LEFT (-1, 0, 1)
	1, 3, 14, 15,	// FRONT RIGHT (1, 0, 1)
	4, 6, 17, 18,	// BACK RIGHT (1, 0, -1)
	7, 9, 20, 21,	// BACK LEFT (-1, 0, -1)
	13, 16, 19, 22	// BOTTOM (0, -1, 0)
};

GLfloat colorList[] = {
	//TOP front = red
	1.0f, 0.0f, 0.0f,	1.0f, 0.0f, 0.0f,	1.0f, 0.0f, 0.0f,
	//TOP right = green
	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,
	//TOP back = blue
	0.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,
	//TOP left = yellow
	1.0f, 1.0f, 0.0f,	1.0f, 1.0f, 0.0f,	1.0f, 1.0f, 0.0f,
	//BOTTOM front = cyan
	0.0f, 1.0f, 1.0f,	0.0f, 1.0f, 1.0f,	0.0f, 1.0f, 1.0f,
	//BOTTOM right = magenta
	1.0f, 0.0f, 1.0f,	1.0f, 0.0f, 1.0f,	1.0f, 0.0f, 1.0f,
	//BOTTOM back = gray
	0.5f, 0.5f, 0.5f,	0.5f, 0.5f, 0.5f,	0.5f, 0.5f, 0.5f,
	//BOTTOM left = orange
	1.0f, 0.65f, 0.0f,	1.0f, 0.65f, 0.0f,	1.0f, 0.65f, 0.0f
};

GLfloat LIGHT[] = { 1.0f, 1.0f, 1.0f, 1.0f };


const char* vertex_shader_src = R"""(
#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

out vec4 vertex_color;
out vec3 camera_direction;
out vec3 fresh_normal;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

void main(){
	//vertex_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex_color = vec4(color.r, color.g, color.b, 1.0f);
	gl_Position = View * Model * vec4(position.x, position.y, position.z, 1.0f);
	camera_direction = normalize(-1.0f * vec3(gl_Position));
	mat3 normalMatrix = mat3(transpose(inverse(View * Model)));
	fresh_normal = normalize(normalMatrix * normal);
	gl_Position = Projection * gl_Position;
}
)""";
const char* fragment_shader_src = R"""(
#version 330

in vec4 vertex_color;
in vec3 camera_direction;  // same as direction to the light since the light is attached to the camera
in vec3 fresh_normal;

out vec4 fragment_color;

uniform float PI;
uniform vec4 LightRGB;

void main(){
	fragment_color = vertex_color / PI * LightRGB * max(dot(normalize(fresh_normal), normalize(camera_direction)), 0.0f);
}
)""";

Renderer::Renderer(GL::platform::Window& window)
	: BasicRenderer(window,3,3)
{
	glClearColor(0.1f, 0.3f, 1.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	window.attach(this);
}

void Renderer::resize(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
}

float Renderer::deg2rad(float degrees) {
	return pi * degrees / 180.0f;
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//addDegree = 0;
	glViewport(0, 0, viewport_width, viewport_height);
	// set the afine transformation vlaues
	float
		// object settings
		tX = 0.0f,
		tY = -0.2f,
		tZ = -1.0f,
		rotX = deg2rad(0.04f*addDegree),
		rotY = deg2rad(-0.08f*addDegree),
		rotZ = deg2rad(0.01f*addDegree),
		sX = 0.25f,
		sY = 0.25f,
		sZ = 0.25f,
		//camera settings
		cameraX = 0.0f,
		cameraY = 0.0f,
		cameraZ = 0.0f,
		lookAtX = tX,
		lookAtY = tY,
		lookAtZ = tZ,
		cameraUpX = 0.0f,
		cameraUpY = 1.0f,
		cameraUpZ = 0.0f,
		// projection settings
		nearFrame = 0.5f,
		farFrame = 5.0f,
		viewAngle = deg2rad(60);

	float sinX = sin(rotX), cosX = cos(rotX),
		sinY = sin(rotY), cosY = cos(rotY),
		sinZ = sin(rotZ), cosZ = cos(rotZ);

	// define the matrices
	math::float4x4 scaleM = math::float4x4(sX, 0, 0, 0, 0, sY, 0, 0, 0, 0, sZ, 0, 0, 0, 0, 1);
	math::float4x4 rotationXM = math::identity<math::float4x4>();
	math::float4x4 rotationYM = math::identity<math::float4x4>();
	math::float4x4 rotationZM = math::identity<math::float4x4>();

	rotationXM._22 = cosX;
	rotationXM._33 = cosX;
	rotationXM._23 = -sinX;
	rotationXM._32 = sinX;

	rotationYM._11 = cosY;
	rotationYM._33 = cosY;
	rotationYM._13 = sinY;
	rotationYM._31 = -sinY;

	rotationZM._11 = cosZ;
	rotationZM._22 = cosZ;
	rotationZM._12 = -sinZ;
	rotationZM._21 = sinZ;

	math::float4x4 modelM = rotationXM * rotationYM * rotationZM * scaleM;
	modelM._14 = tX;
	modelM._24 = tY;
	modelM._34 = tZ;

	//std::cout << modelM << "\n" << std::endl;

	GLfloat modelMGL[4][4] = {
		{ modelM._11, modelM._12, modelM._13, modelM._14 },
		{ modelM._21, modelM._22, modelM._23, modelM._24 },
		{ modelM._31, modelM._32, modelM._33, modelM._34 },
		{ modelM._41, modelM._42, modelM._43, modelM._44 }
	};

	// view matrix
	math::vector<float, 3U> cameraPos = math::vector<float, 3U>(cameraX, cameraY, cameraZ);
	math::vector<float, 3U> W = normalize(cameraPos - math::vector<float, 3U>(lookAtX, lookAtY, lookAtZ));
	math::vector<float, 3U> cameraUP = math::vector<float, 3U>(cameraUpX, cameraUpY, cameraUpZ);
	math::vector<float, 3U> U = normalize(cross(cameraUP, W));
	math::vector<float, 3U> V = cross(W, U);

	GLfloat viewMGL[4][4] = {
		{ U[0], U[1], U[2], -dot(cameraPos, U) },
		{ V[0], V[1], V[2], -dot(cameraPos, V) },
		{ W[0], W[1], W[2], -dot(cameraPos, W) },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};

	// projection matrix
	float aspect =  float(viewport_width) / float(viewport_height),
		tanFieldViewAngle = tan(viewAngle/2.0f);
	GLfloat projectionMGL[4][4] = {
		{ 1.0f / (aspect * tanFieldViewAngle), 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f / tanFieldViewAngle,  0.0f, 0.0f },
		{ 0.0f, 0.0f, -(farFrame+nearFrame)/(farFrame-nearFrame), -(2 * farFrame * nearFrame) / (farFrame - nearFrame)},
		{ 0.0f, 0.0f, -1.0f, 0.0f }
	};

	// calculate the normals
	// 8 triangles * 3 homogenous verteces = 24 verteces (72 floats)
	GLfloat normalList[72];
	for (int triangle = 0, triangleBase; triangle < 8; triangle++) {
		triangleBase = triangle * 9;
		math::vector<float, 3U> vertex1 = math::vector<float, 3U>(vertexList[triangleBase], vertexList[triangleBase + 1], vertexList[triangleBase + 2]);
		math::vector<float, 3U> vertex2 = math::vector<float, 3U>(vertexList[triangleBase+3], vertexList[triangleBase + 4], vertexList[triangleBase + 5]);
		math::vector<float, 3U> vertex3 = math::vector<float, 3U>(vertexList[triangleBase+6], vertexList[triangleBase + 7], vertexList[triangleBase + 8]);
		math::vector<float, 3U> normal = normalize(cross(vertex2 - vertex1, vertex3 - vertex1));
		// save the X coordinate
		normalList[triangleBase] = normal.x;
		normalList[triangleBase + 3] = normal.x;
		normalList[triangleBase + 6] = normal.x;
		// save the Y coordinate
		normalList[triangleBase + 1] = normal.y;
		normalList[triangleBase + 4] = normal.y;
		normalList[triangleBase + 7] = normal.y;
		// save the Z coordinate
		normalList[triangleBase + 2] = normal.z;
		normalList[triangleBase + 5] = normal.z;
		normalList[triangleBase + 8] = normal.z;
	}
	// combine neighbour vertex normals
	/**/
	for (int vertex = 0, vertexBase, indexBase; vertex < 6; vertex++) {
		vertexBase = 4 * vertex;
		math::vector<float, 3U> newNormal = math::vector<float, 3U>(0,0,0);
		for (int indexBase = 0; indexBase < 4; indexBase++) {
			newNormal = newNormal + math::vector<float, 3U>(
				normalList[sameVertexindeces[vertexBase + indexBase]], //   X
				normalList[sameVertexindeces[vertexBase + indexBase]+1], // Y
				normalList[sameVertexindeces[vertexBase + indexBase]+2] //  Z
			);
		}
		newNormal = normalize(newNormal);
		for (int indexBase = 0; indexBase < 4; indexBase++) {
			normalList[sameVertexindeces[vertexBase + indexBase]] = newNormal.x;
			normalList[sameVertexindeces[vertexBase + indexBase] + 1] = newNormal.y;
			normalList[sameVertexindeces[vertexBase + indexBase] + 2] = newNormal.z;
		}
	}

	// the VAO declaration and binding
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create VOB to send vertex and color data
	GLuint vertexVOB, colorVOB, normalVOB;
	// request names, bind for the 1st time, bind the actual data
	glGenBuffers(1, &vertexVOB);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexList), vertexList, GL_STATIC_DRAW);

	glGenBuffers(1, &colorVOB);
	glBindBuffer(GL_ARRAY_BUFFER, colorVOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colorList), colorList, GL_STATIC_DRAW);

	glGenBuffers(1, &normalVOB);
	glBindBuffer(GL_ARRAY_BUFFER, normalVOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normalList), normalList, GL_STATIC_DRAW);

	//configzre VAO layout
	// set position at 0
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set color at 1
	glBindBuffer(GL_ARRAY_BUFFER, colorVOB);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set normals at 2
	glBindBuffer(GL_ARRAY_BUFFER, normalVOB);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
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

	// link matrices to uniforms after load
	GLint modelUniform = GL_SAFE_CALL(glGetUniformLocation(program, "Model"));
	GL_SAFE_CALL(glUniformMatrix4fv(modelUniform, 1, GL_TRUE, *modelMGL));
	GLint viewUniform = GL_SAFE_CALL(glGetUniformLocation(program, "View"));
	GL_SAFE_CALL(glUniformMatrix4fv(viewUniform, 1, GL_TRUE, *viewMGL));
	GLint projectionUniform = GL_SAFE_CALL(glGetUniformLocation(program, "Projection"));
	GL_SAFE_CALL(glUniformMatrix4fv(projectionUniform, 1, GL_TRUE, *projectionMGL));
	// link uniform values to fragment shader
	GLint piUniform = GL_SAFE_CALL(glGetUniformLocation(program, "PI"));
	GL_SAFE_CALL(glUniform1f(piUniform, pi));
	GLint lightUniform = GL_SAFE_CALL(glGetUniformLocation(program, "LightRGB"));
	GL_SAFE_CALL(glUniform4f(lightUniform, LIGHT[0], LIGHT[1], LIGHT[2], LIGHT[3]));

	// start vertex shader to draw the triangles
	GL_SAFE_CALL(glDrawArrays(GL_TRIANGLES, 0, 24));

	swapBuffers();
	addDegree++;
}