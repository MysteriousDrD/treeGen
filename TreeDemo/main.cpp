#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
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
#include <stack>
#include "time.h"


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define STRING_INIT "X"
#define STRING_X "F-[[X]+X]+F[+FX]-X"
#define STRING_F "FF"
#define TREEDEPTH 7
#define ANGLE 25.7
#define GROWTH 0.01
#define BRANCH 0.01

vec3 windInfluence = vec3(0.000, 0.001, 0);
using namespace std;
GLuint shaderProgramID;

vector<float>  rPts;
vector<unsigned int> vaos;
unsigned int tex = 0;
unsigned int teapot_vao = 0;
int width = 1920.0;
int height = 1080.0;
int nVertices = 0;
vector<int> branches;
int branchCount = 0;
#pragma region SHADER_FUNCTIONS

struct turtle{
	float x;
	float y;
	float angle;

};

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

#pragma region VBO_FUNCTIONS

void generateObjectBuffers(vector<vector <float>> rotations, GLfloat colors[])
{
	for(int i = 0; i < rotations.size(); i++)
	{

		GLuint loc1 = glGetAttribLocation(shaderProgramID, "vPosition");
		GLuint loc2 = glGetAttribLocation(shaderProgramID, "vColor");
		GLuint numVertices = nVertices;
		GLuint vp_vbo, vc_vbo;

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glBufferData(GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), rotations[i].data(), GL_STATIC_DRAW);

		glGenBuffers(1, &vc_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vc_vbo);
		glBufferData(GL_ARRAY_BUFFER, numVertices*4*sizeof(GLfloat), colors, GL_STATIC_DRAW);

		unsigned int vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vc_vbo);
		glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		vaos.push_back(vao);
	}
	cout << vaos.size() << endl;
}


GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = nVertices;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*7*sizeof(GLfloat), NULL, GL_STATIC_DRAW); //this is 6 because x,y and 4 colour params
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*3*sizeof(GLfloat), vertices); //0 -> number of vertices
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors); //number of vertices -> end of colours

	return VBO;
}

void _linkCurrentBuffertoShader(GLuint shaderProgramID){
	GLuint numVertices = nVertices;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*3*sizeof(GLfloat)));
}


#pragma endregion VBO_FUNCTIONS
float pan = 0.0f;
float direction = 0.1f;
int inc = 0;
int old = 0;
float foo = 0;
int angleOfRotation = 0;

float targX = 1;
float targY = 0;
float targZ = 1;

void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);
	vec3 camera = vec3(targX,targY,targZ);
	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");

	
	mat4 view = translate (identity_mat4 (), vec3 (0.0, 0.0, -40));
	mat4 view2 = look_at(camera, vec3(0,0,1), vec3(0,1,0));
	view = view*view2;
	mat4 persp_proj = perspective(90, (float)width/(float)height, 0.1, 100.0);
	mat4 model = scale(identity_mat4(), vec3(20,20,20));
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, model.m);
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);

	glBindVertexArray(vaos[foo]);

	glDrawArrays(GL_LINE_STRIP, 0, nVertices);

	mat4 models[18];
	for(int i = 0; i < 12; i++)
	{
		models[i] = identity_mat4();
		models[i] = scale(models[i], vec3(20,20,20));
		models[i] = rotate_y_deg(models[i], 30*i);
		glUniformMatrix4fv (matrix_location, 1, GL_FALSE, models[i].m);
		glDrawArrays(GL_LINE_STRIP, 0, nVertices);
	}
    glutSwapBuffers();
}


string treeSystem(string tree, int depth)
{
	

	if(depth == TREEDEPTH) return tree;
	string tmpTree = tree;
	string newTree = "";

	for(int i = 0; i < tmpTree.size(); i++)
	{
		if(tmpTree[i] == 'X')
		{
			newTree += STRING_X; // first rule of L System
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
	stack<turtle> turtles;
	turtle leonardo ={};
	leonardo.angle = 90;
	float lastX = 0;
	float lastY = 0;
	for(int i = 0; i < tree.size(); i++)
	{
		if(tree[i] == 'F')
		{
			
			//currY += GROWTH
			float tmpX = (leonardo.x + BRANCH * cos(leonardo.angle));
			float tmpY =  (leonardo.y + BRANCH * sin(leonardo.angle));
			lastX = leonardo.x;
			lastY = leonardo.y;
			leonardo.x = tmpX +windInfluence.v[0];
			leonardo.y = tmpY +windInfluence.v[1];	


			//cout << "moving forward to " << currX << " , " << currY<< endl;

			points.push_back(leonardo.x);
			points.push_back(leonardo.y);
			points.push_back(0);

			nVertices++;
		}
		if(tree[i] =='+')
		{
			leonardo.angle -= ANGLE;
			branchCount++;
		}
		if(tree[i] == '-')
		{
			leonardo.angle += ANGLE;
			branchCount++;
		}
		if(tree[i] == '[')
		{
			turtles.push(leonardo);
		}
		if(tree[i] == ']')
		{
			
			leonardo = turtles.top();
			turtles.pop();
			branches.push_back(nVertices);


		}
		
	}
	//cout << "Vertices: " <<  nVertices << endl;
	//cout << branches.size();
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

void keypress(unsigned char key, int x, int y) {
        if(key=='w')
		{
			targX +=1; 
        }
		if(key=='s')
		{
			targX -=1;
		}
		if(key == 'i')
		{
			foo += 1;
			if(foo == 17) foo = 0;
			cout << foo << endl;
		}
		if(key == 'a')
		{
			targY += 1;
		}
		if(key == 'd')
		{
			targY -= 1;
		}
		if(key == 'q')
		{
			targZ += 1;
		}
		if(key == 'e')
		{
			targZ -= 1;
		}
}



void generateObjectBufferBillboards(GLuint shaderProgramID, GLfloat vertices[], GLfloat texcoords[])
{
	GLuint dimensions = 2;
	GLuint length = 6;
	GLuint numVertices = 6;
	GLuint VBO;
	GLuint vt_vbo;
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint textureID = glGetAttribLocation(shaderProgramID, "vt");
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vt_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glBufferData(GL_ARRAY_BUFFER, dimensions*length*sizeof(GLfloat), texcoords, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(positionID);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE,0, NULL);

	glEnableVertexAttribArray(textureID);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer(textureID, 2, GL_FLOAT, GL_FALSE, 0, NULL);


}


vector<float> rotateTree(GLfloat *skeleton, float angle)
{
	angle = angle * ONE_DEG_IN_RAD;
	vector<float> newTree;
	for(int i = 0; i < nVertices*3; i+=3)
	{
		float x = skeleton[i];
		float y = skeleton[i+1];
		float z = skeleton[i+2];

		float newZ = z*cos(angle) - x*sin(angle);
		float newX = z*sin(angle) + x*cos(angle);
		float newY = y;

		newTree.push_back(newX);
		newTree.push_back(newY);
		newTree.push_back(newZ);

	}
	return newTree;

}


void linkCurrentBuffertoShader(GLuint shaderProgramID){
 
       glBindAttribLocation (shaderProgramID, 0, "vPosition");
       glBindAttribLocation (shaderProgramID, 1, "vColor");
      
}


void init()
{

	string tree =  treeSystem("X", 0);
	vector<float> pts = walkTree(tree);
	cout << branchCount << endl;
	cout << pts.size() << endl;
	GLfloat *vertices = pts.data();
	cout << nVertices << endl;
	rPts = rotateTree(vertices, 360);
	GLfloat *rVertices = rPts.data();
	vector<vector <float>> rotations;
	rotations.push_back(pts); //first unrotated tree
	for(int i = 20; i < 360; i+=20)
	{
		rotations.push_back(rotateTree(vertices,i));
	}
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	cout << rotations.size() << endl;
	vector<float> cols;
	for(int i = 0; i < nVertices; i++)
	{
		cols.push_back(0); //r
		cols.push_back(1); //g
		cols.push_back(0); //b
		cols.push_back(0); //a
	}
	GLfloat *colors = cols.data();
	/*GLfloat texcoords[] = {
	  0.0f, 1.0f,
	  0.0f, 0.0f,
	  1.0, 0.0,
	  1.0, 0.0,
	  1.0, 1.0,
	  0.0, 1.0
	};

	GLfloat vertices[] = {0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		0.0f, 1.0, 0.0f,
	};*/
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	//generateObjectBufferBillboards(shaderProgramID, vertices, texcoords);
	generateObjectBuffers(rotations, colors);
	_linkCurrentBuffertoShader(shaderProgramID);
	//c.generateVertices(10.0, 5.0, 5.0, vec4(0.32,0.19,0.09,1.0), vec4(0.32,0.19,0.09,1.0),16);
    //c.generateObjectBuffer();
	//load_image_to_texture("tree.png", tex, true);


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
	glutKeyboardFunc(keypress);

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










