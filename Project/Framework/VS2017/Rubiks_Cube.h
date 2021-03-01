#pragma once
#include "Cube.h"
#include <array>
#include <vector>

using namespace glm;
using namespace std;

class Rubiks_Cube {
private:
	//array<array<array<Cube, 3>, 3>, 3> cubeArray;
	vector<Cube> cubeArray;
	vec3 position = vec3(0.0f);
	vector<GLuint> texturePack;
	vec3 rotateFactor = vec3(0.0f);
	bool won = false;

public:
	Rubiks_Cube();
	Rubiks_Cube(vec3 pos, vector<GLuint> texture);

	void generateCube(Shader S);
	void setShader(Shader S);
	void drawModel();

	void rotate_y(string direction, int y);

	void rotate_x(string direction, int x);

	void rotate_z(string direction, int z);

	void setPosition(vec3 pos);
	vec3 getPosition();

	void resetPosition();
	void randomizePosition(int iteration);

	void testforWinCondition();
	bool getWin();

	void debug();
};