
#include "Renderer.h"
#include "iostream"
#include "framework\png.h"

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

// for memorz leaks moving the declarations of matricesand vectors here
math::float4x4 scaleM = math::identity<math::float4x4>();
math::float4x4 rotationXM = math::identity<math::float4x4>();
math::float4x4 rotationYM = math::identity<math::float4x4>();
math::float4x4 rotationZM = math::identity<math::float4x4>();
math::float4x4 modelM;
math::float3 cameraPos, W, cameraUP, U, V;

int totalVertexCount, totalVertexFloatCount, totalTextureUVFloatCount;

// starting afine transformation settings
float
// object settings
tX = 0.0f,
tY = 0.0f,
tZ = -1.5f,
rotX = 0,
rotY = 0,
rotZ = 0,
sX = 0.15f,
sY = 0.15f,
sZ = 0.15f,
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
farFrame = 5.0f;

GLfloat projectionMGL[4][4] = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f,  0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, -1.0f, 0.0f }
};

GLfloat viewMGL[4][4] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};

GLfloat modelMGL[4][4] = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};


// Load image for texture
//const char* textureOBJFile = "..\\assets\\cube.obj";
//const char* texturePNGFile = "..\\assets\\Red-brick-wall.png";
//const char* textureOBJFile = "..\\assets\\desert.obj";
//const char* texturePNGFile = "..\\assets\\sand.png";
//const char* textureOBJFile = "..\\assets\\nukahedron.obj";
//const char* texturePNGFile = "..\\assets\\nukahedron_diffuse.png";
const char* textureOBJFile = "assets\\vader.obj";
const char* texturePNGFile = "assets\\vader.png";

const char* vertex_shader_src = R"""(
#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texUV;

out vec2 vertex_texture_UV;
out vec3 camera_direction;
out vec3 fresh_normal;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

void main(){
	gl_Position = View * Model * vec4(position.x, position.y, position.z, 1.0f);
	camera_direction = normalize(-1.0f * vec3(gl_Position));
	mat3 normalMatrix = mat3(transpose(inverse(View * Model)));
	fresh_normal = normalize(normalMatrix * normal);

	gl_Position = Projection * gl_Position;
	vertex_texture_UV = texUV;
}
)""";

 const char* fragment_shader_src = R"""(
#version 330

in vec2 vertex_texture_UV;
in vec3 camera_direction;  // same as direction to the light since the light is attached to the camera
in vec3 fresh_normal;

out vec4 fragment_color;

uniform sampler2D faceTex;
uniform float PI;
uniform vec4 LightRGB;

void main(){
//	fragment_color = texture(faceTex, vertex_texture_UV);
//	fragment_color = vec4(1.0f, 1.0f, 1.0f, 1.0f) / PI * LightRGB * max(dot(normalize(fresh_normal), normalize(camera_direction)), 0.0f);
	fragment_color = texture(faceTex, vertex_texture_UV) / PI * LightRGB * max(dot(normalize(fresh_normal), normalize(camera_direction)), 0.0f);
}
)""";

Renderer::Renderer(GL::platform::Window& window)
	: BasicRenderer(window, 3, 3)
{
	glClearColor(0.1f, 0.3f, 1.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	int maxV = 0,
		maxUV = 0,
		maxN = 0,
		maxF = 0,
		VIC = 0,
		UVIC = 0,
		NIC = 0,
		FIC = 0;

	// load OBJ file just for getting the table sizes a.k.a. first run
	char * line = new char[200], *token1, *token2, *token3;
	std::ifstream OBJ_FILE(textureOBJFile);
	while (!OBJ_FILE.eof()) {
		OBJ_FILE.getline(line, 200);
		if (std::strncmp(line, "f ", 2) == 0) {
			maxF += 1;

			std::strtok(line, " ");	// remove the f
			token1 = std::strtok(NULL, " "); // get 1st triplet of vertex/texture/normal
			token2 = std::strtok(NULL, " "); // get 2nd triplet of vertex/texture/normal
			token3 = std::strtok(NULL, " "); // get 3rd triplet of vertex/texture/normal

			// check indecies for 1st vertex
			maxV = math::max(maxV, (int) atof(std::strtok(token1, "/"))); // check vertex index
			maxUV = math::max(maxUV, (int)atof(std::strtok(NULL, "/"))); // check texture index
			maxN = math::max(maxN, (int)atof(std::strtok(NULL, "/"))); // check normal index
			// check indecies for 2nd vertex
			maxV = math::max(maxV, (int)atof(std::strtok(token2, "/"))); // check vertex index
			maxUV = math::max(maxUV, (int)atof(std::strtok(NULL, "/"))); // check texture index
			maxN = math::max(maxN, (int)atof(std::strtok(NULL, "/"))); // check normal index
			// check indecies for 3rd vertex
			maxV = math::max(maxV, (int)atof(std::strtok(token3, "/"))); // check vertex index
			maxUV = math::max(maxUV, (int)atof(std::strtok(NULL, "/"))); // check texture index
			maxN = math::max(maxN, (int)atof(std::strtok(NULL, "/"))); // check normal index
		}
	}
	std::cout << "Max indeces of f statements: V[" << maxV << "], UV[" << maxUV << "], N[" << maxN << "], f[" << maxF << "]" << std::endl;

	// load OBJ file
	float **OBJ_VERTICES = new float* [3];
	float **OBJ_NORMALS = new float*[3];
	float **OBJ_TEXUV = new float*[2];
	int **OBJ_TRIANGLE_VI = new int*[3];
	int **OBJ_TRIANGLE_NI = new int*[3];
	int **OBJ_TRIANGLE_TI = new int*[3];
	for (int axis = 0; axis < 3; axis++) {
		if (axis < 2) {
			OBJ_TEXUV[axis] = new float[maxUV];
		}
		OBJ_VERTICES[axis] = new float[maxV];
		OBJ_NORMALS[axis] = new float[maxN];
		OBJ_TRIANGLE_VI[axis] = new int[maxF];
		OBJ_TRIANGLE_NI[axis] = new int[maxF];
		OBJ_TRIANGLE_TI[axis] = new int[maxF];
	}

	// set the reader back to start
	OBJ_FILE.clear();
	OBJ_FILE.seekg(0, std::ios::beg);
	while(!OBJ_FILE.eof()){
		OBJ_FILE.getline(line,100);
		if (std::strncmp(line, "v ", 2) == 0) {
//			std::cout << line << std::endl;
			std::strtok(line, " ");	// remove the v
			OBJ_VERTICES[0][VIC] = (float)atof(std::strtok(NULL, " ")); // get the X
			OBJ_VERTICES[1][VIC] = (float)atof(std::strtok(NULL, " ")); // get the Y
			OBJ_VERTICES[2][VIC] = (float)atof(std::strtok(NULL, " ")); // get the Z
			VIC += 1;
		}
		else if (std::strncmp(line, "vn", 2) == 0) {
	//		std::cout << line << std::endl;
			std::strtok(line, " ");	// remove the vn
			OBJ_NORMALS[0][NIC] = (float)atof(std::strtok(NULL, " ")); // get the X
			OBJ_NORMALS[1][NIC] = (float)atof(std::strtok(NULL, " ")); // get the Y
			OBJ_NORMALS[2][NIC] = (float)atof(std::strtok(NULL, " ")); // get the Z
			NIC += 1;
		}
		else if (std::strncmp(line, "vt", 2) == 0) {
//			std::cout << line << std::endl;
			std::strtok(line, " ");	// remove the vt
			OBJ_TEXUV[0][UVIC] = (float)atof(std::strtok(NULL, " ")); // get the U
			OBJ_TEXUV[1][UVIC] = 1.0f - (float)atof(std::strtok(NULL, " ")); // get the V (image height goes other waz, therefore 1 - value makes it correct)
			UVIC += 1;
		}
		else if (std::strncmp(line, "f ", 2) == 0) {
	//		std::cout << line << std::endl;
			std::strtok(line, " ");	// remove the f
			token1 = std::strtok(NULL, " "); // get 1st triplet of vertex/texture/normal
			token2 = std::strtok(NULL, " "); // get 2nd triplet of vertex/texture/normal
			token3 = std::strtok(NULL, " "); // get 3rd triplet of vertex/texture/normal

			// save for 1st vertex
			OBJ_TRIANGLE_VI[0][FIC] = (int)atof(std::strtok(token1, "/")) - 1;	//save the vertex in the vertices
			OBJ_TRIANGLE_TI[0][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the texture in the textures
			OBJ_TRIANGLE_NI[0][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the normal in the normals

			// save for 2nd vertex
			OBJ_TRIANGLE_VI[1][FIC] = (int)atof(std::strtok(token2, "/")) - 1;	//save the vertex in the vertices
			OBJ_TRIANGLE_TI[1][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the texture in the textures
			OBJ_TRIANGLE_NI[1][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the normal in the normals

			// save for 3rd vertex
			OBJ_TRIANGLE_VI[2][FIC] = (int)atof(std::strtok(token3, "/")) - 1;	//save the vertex in the vertices
			OBJ_TRIANGLE_TI[2][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the texture in the textures
			OBJ_TRIANGLE_NI[2][FIC] = (int)atof(std::strtok(NULL, "/")) - 1;		//save the normal in the normals

			FIC += 1;
		}
	}
	OBJ_FILE.close();

	std::cout << "Index count during data copying:  V[" << VIC << "], UV[" << UVIC << "], N[" << NIC << "], f[" << FIC << "]" << std::endl;

	//Prepare the data in right format
	totalVertexCount = FIC * 3; // number of F definitions * 3 triangle vertecies
	std::cout << totalVertexCount << std::endl;
	totalVertexFloatCount = totalVertexCount * 3;  // number of F definitions * 3 triangle vertecies * 3 values for vertex
	totalTextureUVFloatCount = totalVertexCount * 2;  // number of F definitions * 3 triangle vertecies * 2 values for vertex
	GLfloat *vertexList = NULL, *normalList = NULL, *textureList = NULL;
	vertexList = new GLfloat[totalVertexFloatCount];
	normalList = new GLfloat[totalVertexFloatCount];
	textureList = new GLfloat[totalTextureUVFloatCount];

	for (int triangle = 0, row; triangle < FIC; triangle++) {
		for (int vertex = 0; vertex < 3; vertex++) {
			// row of the table if each row has the whole triangle
			row = triangle * 9;
			for (int axis = 0; axis < 3; axis++) {
				// load the 3 floats for each of 3 vertexes of each of the triangles
				vertexList[row + axis + (vertex*3)] = OBJ_VERTICES[axis][OBJ_TRIANGLE_VI[vertex][triangle]];
				normalList[row + axis + (vertex*3)] = OBJ_NORMALS[axis][OBJ_TRIANGLE_NI[vertex][triangle]];
			}
			// row of the table if each row has the whole triangle UV
			row = triangle * 6;
			for (int axis = 0; axis < 2; axis++) {
				// load the 2 floats for each of 3 vertexes of each of the triangles
				textureList[row + axis + (vertex * 2)] = OBJ_TEXUV[axis][OBJ_TRIANGLE_TI[vertex][triangle]];
			}
		}
	}

	// free memory in arrays
	for (int i = 0; i < 3; i++) {
		if (i < 2) {
			delete[] OBJ_TEXUV[i];
		}
		delete[] OBJ_VERTICES[i];
		delete[] OBJ_NORMALS[i];
		delete[] OBJ_TRIANGLE_VI[i];
		delete[] OBJ_TRIANGLE_NI[i];
		delete[] OBJ_TRIANGLE_TI[i];
	}
	delete[] OBJ_VERTICES;
	delete[] OBJ_NORMALS;
	delete[] OBJ_TEXUV;
	delete[] OBJ_TRIANGLE_VI;
	delete[] OBJ_TRIANGLE_NI;
	delete[] OBJ_TRIANGLE_TI;

	// the VAO declaration and binding
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create VOB to send vertex and color data
	GLuint vertexVOB, normalVOB, textUVVOB;
	// request names, bind for the 1st time, bind the actual data
	// the 4 * float count is since we have 32bit(4B) floats
	glGenBuffers(1, &vertexVOB);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glBufferData(GL_ARRAY_BUFFER, 4 * totalVertexFloatCount, vertexList, GL_STATIC_DRAW);

	glGenBuffers(1, &normalVOB);
	glBindBuffer(GL_ARRAY_BUFFER, normalVOB);
	glBufferData(GL_ARRAY_BUFFER, 4 * totalVertexFloatCount, normalList, GL_STATIC_DRAW);

	glGenBuffers(1, &textUVVOB);
	glBindBuffer(GL_ARRAY_BUFFER, textUVVOB);
	glBufferData(GL_ARRAY_BUFFER, 4 * totalTextureUVFloatCount, textureList, GL_STATIC_DRAW);

	//configzre VAO layout
	// set position at 0
	glBindBuffer(GL_ARRAY_BUFFER, vertexVOB);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set normals at 1
	glBindBuffer(GL_ARRAY_BUFFER, normalVOB);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);;
	// set textUV at 2
	glBindBuffer(GL_ARRAY_BUFFER, textUVVOB);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// bind VAO
	glBindVertexArray(vao);

	// adding textures to the model
	image<std::uint32_t> textureImage(PNG::loadImage2D(texturePNGFile));
	GLuint texture;
	GL_SAFE_CALL(glGenTextures(1, &texture));
	GL_SAFE_CALL(glBindTexture(GL_TEXTURE_2D, texture));
	GL_SAFE_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(textureImage), height(textureImage), 0, GL_RGBA, GL_UNSIGNED_BYTE, data(textureImage)));
	GL_SAFE_CALL(glGenerateMipmap(GL_TEXTURE_2D));

	//clear arrays
	delete[] vertexList;
	delete[] normalList;
	delete[] textureList;

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
	rotX = deg2rad(0.04f*addDegree);
	rotY = deg2rad(-0.08f*addDegree);
	rotZ = deg2rad(0.01f*addDegree);
	

	float sinX = sin(rotX), cosX = cos(rotX),
		sinY = sin(rotY), cosY = cos(rotY),
		sinZ = sin(rotZ), cosZ = cos(rotZ);

	// define the matrices
	scaleM = math::float4x4(sX, 0, 0, 0, 0, sY, 0, 0, 0, 0, sZ, 0, 0, 0, 0, 1);


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

	modelM = rotationXM * rotationYM * rotationZM * scaleM;
	modelM._14 = tX;
	modelM._24 = tY;
	modelM._34 = tZ;

	//std::cout << modelM << "\n" << std::endl;

	modelMGL[0][0] = modelM._11;
	modelMGL[0][1] = modelM._12;
	modelMGL[0][2] = modelM._13;
	modelMGL[0][3] = modelM._14;
	modelMGL[1][0] = modelM._21;
	modelMGL[1][1] = modelM._22;
	modelMGL[1][2] = modelM._23;
	modelMGL[1][3] = modelM._24;
	modelMGL[2][0] = modelM._31;
	modelMGL[2][1] = modelM._32;
	modelMGL[2][2] = modelM._33;
	modelMGL[2][3] = modelM._34;
	modelMGL[3][0] = modelM._41;
	modelMGL[3][1] = modelM._42;
	modelMGL[3][2] = modelM._43;
	modelMGL[3][3] = modelM._44;

//	GLfloat modelMGL[4][4] = {
//		{ modelM._11, modelM._12, modelM._13, modelM._14 },
//		{ modelM._21, modelM._22, modelM._23, modelM._24 },
//		{ modelM._31, modelM._32, modelM._33, modelM._34 },
//		{ modelM._41, modelM._42, modelM._43, modelM._44 }
//	};

	// view matrix
	cameraPos = math::float3(cameraX, cameraY, cameraZ);
	W = normalize(cameraPos - math::float3(lookAtX, lookAtY, lookAtZ));
	cameraUP = math::float3(cameraUpX, cameraUpY, cameraUpZ);
	U = normalize(cross(cameraUP, W));
	V = cross(W, U);

	viewMGL[0][0] = U[0];
	viewMGL[0][1] = U[1];
	viewMGL[0][2] = U[2];
	viewMGL[0][3] = -dot(cameraPos, U);

	viewMGL[1][0] = V[0];
	viewMGL[1][1] = V[1];
	viewMGL[1][2] = V[2];
	viewMGL[1][3] = -dot(cameraPos, V);

	viewMGL[2][0] = W[0];
	viewMGL[2][1] = W[1];
	viewMGL[2][2] = W[2];
	viewMGL[2][3] = -dot(cameraPos, W);

//	GLfloat viewMGL[4][4] = {
//		{ U[0], U[1], U[2], -dot(cameraPos, U) },
//		{ V[0], V[1], V[2], -dot(cameraPos, V) },
//		{ W[0], W[1], W[2], -dot(cameraPos, W) },
//		{ 0.0f, 0.0f, 0.0f, 1.0f }
//	};

	// projection matrix
	float viewAngle = deg2rad(60),
		aspect = float(viewport_width) / float(viewport_height),
		tanFieldViewAngle = tan(viewAngle / 2.0f);

	projectionMGL[0][0] = 1.0f / (aspect * tanFieldViewAngle);
	projectionMGL[1][1] = 1.0f / (aspect * tanFieldViewAngle);
	projectionMGL[2][2] = -(farFrame + nearFrame) / (farFrame - nearFrame);
	projectionMGL[2][3] = -(2 * farFrame * nearFrame) / (farFrame - nearFrame);
		
//		[4][4] = {
//		{ 1.0f / (aspect * tanFieldViewAngle), 0.0f, 0.0f, 0.0f },
//		{ 0.0f, 1.0f / tanFieldViewAngle,  0.0f, 0.0f },
//		{ 0.0f, 0.0f, -(farFrame + nearFrame) / (farFrame - nearFrame), -(2 * farFrame * nearFrame) / (farFrame - nearFrame) },
//		{ 0.0f, 0.0f, -1.0f, 0.0f }
//	};

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
	GL_SAFE_CALL(glUniform4f(lightUniform, 1.0f, 1.0f, 1.0f, 1.0f));

	// start vertex shader to draw the triangles
	GL_SAFE_CALL(glDrawArrays(GL_TRIANGLES, 0, totalVertexCount));

	swapBuffers();
	addDegree++;
}