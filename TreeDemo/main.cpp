
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include "maths_funcs.h" //Anton's math class
#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "time.h"
// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int teapot_vao = 0;
int width = 1920.0;
int height = 1080.0;
int nVertices = 0;
// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName);
	if(file.fail())
		return false;
	
	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);

	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = nVertices;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*6*sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*2*sizeof(GLfloat), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*2*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors);

	return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID){
	GLuint numVertices = nVertices;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
	glVertexAttribPointer(positionID, 2, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*2*sizeof(GLfloat)));
}

#pragma endregion VBO_FUNCTIONS
float pan = 0.0f;
float direction = 0.1f;
void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	
	mat4 view = translate (identity_mat4 (), vec3 (0.0, 0.0, -40.0));
	mat4 persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 100.0);
	mat4 model = rotate_z_deg (identity_mat4 (), 45);

	glDrawArrays(GL_LINES, 0, nVertices);

    glutSwapBuffers();
}

string treeSystem(string tree, int depth)
{
	

	if(depth == 1) return tree;
	string tmpTree = tree;
	string newTree = "";

	for(int i = 0; i < tmpTree.size(); i++)
	{
		if(tmpTree[i] == 'X')
		{
			newTree += " F-[[X]+X]+F[+FX]-X"; // first rule of L System
		}
		else if(tmpTree[i] == 'F')
		{
			newTree+= "FF";
		}
		else
		{
			newTree += tree[i];
		}
	}

	int newDepth = depth+1;
	return treeSystem(newTree, newDepth);
	

}

vector<float> walkTree(string tree)
{
	//cout << tree << endl;
	
	vector<float> points;
	float currX = 0.25;
	float currY = 0;
	float tmpX = 0;
	float tmpY = 0;
	float angle = 0.25;
	for(int i = 0; i < tree.size(); i++)
	{
		if(tree[i] == 'F')
		{
			//	cout << "moving forward" << endl;
			currY += 0.1;
			points.push_back(currX);
			points.push_back(currY);
			nVertices++;
		}
		if(tree[i] =='+')
		{
			//	cout << "moving left" << endl;
			currX -= angle;
			points.push_back(currX);
			points.push_back(currY);
			nVertices++;
		}
		if(tree[i] == '-')
		{
			//	cout << "moving right" << endl;
			currX += angle;
			points.push_back(currX);
			points.push_back(currY);
			nVertices++;
		}
		if(tree[i] == '[')
		{
			tmpX = currX;
			tmpY = currY;
		}
		if(tree[i] == ']')
		{
			currX = tmpX;
			currY = tmpY;
		}


	}
	cout << "Vertices: " <<  nVertices << endl;
	return points;
}


void updateScene() {	

		// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	//GLfloat vertices[] = {-1.0f, -1.0f, 0.0f, 1.0,
	//		1.0f, -1.0f, 0.0f, 1.0, 
	//		0.0f, 1.0f, 0.0f, 1.0};
	string tree =  treeSystem("X", 0);
	vector<float> pts = walkTree(tree);

	GLfloat *vertices = pts.data();

	for(int i = 0; i < pts.size(); i += 2)
	{
		
		cout << vertices[i] << ",";
		cout << vertices[i+1] << ",";
		cout << endl;
	}
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)

	vector<float> cols;
	for(int i = 0; i < nVertices; i++)
	{
		cols.push_back(0); //r
		cols.push_back(1); //g
		cols.push_back(0); //b
		cols.push_back(0); //a
	}
	GLfloat *colors = cols.data();
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	generateObjectBuffer(vertices, colors);
	linkCurrentBuffertoShader(shaderProgramID);
}

int main(int argc, char** argv){


	srand(time(NULL));
	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Tree Demo");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











