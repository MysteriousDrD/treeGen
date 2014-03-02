#define _CRT_SECURE_NO_WARNINGS
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
#include "stb_image.h"

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#include "cylinder.h"
#define TREEDEPTH 7
#define ANGLE 25.7
#define GROWTH 0.01
#define BRANCH 0.01
using namespace std;
GLuint shaderProgramID;

unsigned int tex = 0;
Cylinder c;
unsigned int teapot_vao = 0;
int width = 1920.0;
int height = 1080.0;
int nVertices = 0;
vector<int> branches;
int branchCount = 0;
#pragma region SHADER_FUNCTIONS
bool load_image_to_texture(const char* file_name, unsigned int& tex, bool gen_mips);

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

GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = nVertices;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*6*sizeof(GLfloat), NULL, GL_STATIC_DRAW); //this is 6 because x,y and 4 colour params
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*2*sizeof(GLfloat), vertices); //0 -> number of vertices
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*2*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors); //number of vertices -> end of colours

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
	glVertexAttribPointer(positionID, 2, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*2*sizeof(GLfloat)));
}


#pragma endregion VBO_FUNCTIONS
float pan = 0.0f;
float direction = 0.1f;
int inc = 0;
int old = 0;
float foo = 0;
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


	mat4 view = translate (identity_mat4 (), vec3 (0.0, 0.0, -40));
	mat4 persp_proj = perspective(90, (float)width/(float)height, 0.1, 100.0);
	mat4 model = scale(identity_mat4(), vec3(10,10,10));
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, model.m);
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);


	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	inc+= 10;

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
			newTree += "F-[[X]+X]+F[+FX]-X"; // first rule of L System
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
			leonardo.x = tmpX;
			leonardo.y = tmpY;	


			//cout << "moving forward to " << currX << " , " << currY<< endl;
			points.push_back(leonardo.x);
			points.push_back(leonardo.y);

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
          inc = 0;    
        }
		if(key=='q')
		{
			inc = nVertices;
			cout << nVertices << endl;
		}
		if(key == 'i')
		{
			foo += 1;
		}
		if(key == 'k')
		{
			foo -= 1;
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



void linkCurrentBuffertoShader(GLuint shaderProgramID){
 
       glBindAttribLocation (shaderProgramID, 0, "vPosition");
       glBindAttribLocation (shaderProgramID, 1, "vColor");
      
}


void init()
{

	string tree =  treeSystem("X", 0);
	vector<float> pts = walkTree(tree);
	cout << branchCount << endl;
	/*GLfloat *vertices = pts.data();

	// Create a color array that identfies the colors of each vertex (format R, G, B, A)

	vector<float> cols;
	for(int i = 0; i < nVertices; i++)
	{
		cols.push_back(0); //r
		cols.push_back(1); //g
		cols.push_back(0); //b
		cols.push_back(0); //a
	}
	GLfloat *colors = cols.data();*/
	GLfloat texcoords[] = {
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
	};
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	generateObjectBufferBillboards(shaderProgramID, vertices, texcoords);
	
	//c.generateVertices(10.0, 5.0, 5.0, vec4(0.32,0.19,0.09,1.0), vec4(0.32,0.19,0.09,1.0),16);
    //c.generateObjectBuffer();
	load_image_to_texture("tree.png", tex, true);


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

bool load_image_to_texture (
	const char* file_name, unsigned int& tex, bool gen_mips
) {
	printf ("loading image %s\n", file_name);
	int x, y, n;
	int force_channels = 4;
	unsigned char* image_data = stbi_load (file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf (
			stderr,
			"ERROR: could not load image %s. Check file type and path\n",
			file_name
		);
		fprintf (stderr, "ERROR: could not load image %s", file_name);
		return false;
	}
	printf ("image loaded: %ix%i %i bytes per pixel\n", x, y, n);
	// NPOT check
	if (x & (x - 1) != 0 || y & (y - 1) != 0) {
		fprintf (
			stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
		);
		fprintf (stderr, "WARNING: texture %s is not power-of-two dimensions", file_name);
	}

	// FLIP UP-SIDE DIDDLY-DOWN
	// make upside-down copy for GL
	unsigned char *imagePtr = &image_data[0];
	int halfTheHeightInPixels = y / 2;
	int heightInPixels = y;

	// Assuming RGBA for 4 components per pixel.
	int numColorComponents = 4;
	// Assuming each color component is an unsigned char.
	int widthInChars = x * numColorComponents;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	for( int h = 0; h < halfTheHeightInPixels; h++) {
		top = imagePtr + h * widthInChars;
		bottom = imagePtr + (heightInPixels - h - 1) * widthInChars;
		for (int w = 0; w < widthInChars; w++) {
			// Swap the chars around.
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			++top;
			++bottom;
		}
	}

	// Copy into an OpenGL texture
	glGenTextures (1, &tex);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, tex);
	//glPixelStorei (GL_UNPACK_ALIGNMENT, 1); // TODO?
	glTexImage2D (
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
	// NOTE: need this or it will not load the texture at all
	if (gen_mips) {
		// shd be in core since 3.0 according to:
		// http://www.opengl.org/wiki/Common_Mistakes#Automatic_mipmap_generation
		// next line is to circumvent possible extant ATI bug
		// but NVIDIA throws a warning glEnable (GL_TEXTURE_2D);
		glGenerateMipmap (GL_TEXTURE_2D);
		printf ("mipmaps generated %s\n", file_name);
		glTexParameteri (
			GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
		);
		glTexParameterf (
			GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4
		);
	} else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	printf ("image loading done");
	stbi_image_free (image_data);

	return true;
}









