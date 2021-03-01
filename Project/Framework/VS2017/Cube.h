#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include "shader_compile.h"

#pragma once

using namespace glm;

class Cube
{
private:
	vec3 size = vec3(1.0f);
	vec3 basePosition = vec3(1.0f);

	mat4 baseRotationMatrix = rotate(mat4(1.0f), radians(0.0f), vec3(0.0f, 1.0f, 0.0f));

	mat4 customTranslation = mat4(1.0f);
	mat4 customRotation = mat4(1.0f);
	mat4 modelMatrix = mat4(1.0f);
	float currentScaleFactor = 1.0f;

	Shader currentShader;

	vec3 positionTag = vec3(0.0f); //specifically for rubik's cube 
	vec3 initialPositionTag = vec3(0.0f); //for drawing 


public: 
	Cube();
	Cube(vec3 basePos, Shader S);

	void setDefaultSize(vec3 s);
	void setDefaultRotation(mat4 rotationMatrix);
	void setDefaultPosition(vec3 pos);

	void update();

	void setCustomTranslation(vec3 offset);
	void setCustomScaling(float scaleFactor);

	void setCurrentShader(Shader S);

	void setPositionTag(vec3 tag);
	vec3 getPositionTag();

	void setInitialPositionTag(vec3 tag);
	vec3 getInitialPositionTag();

	void setDepthMap(mat4 lightProjMatrix, mat4 lightViewMatrix);
	void drawLeftFace();
	void drawRightFace();
	void drawFrontFace();
	void drawBackFace();
	void drawTopFace();
	void drawBotFace();
	void drawModel();

	//specifically for rubik's cube 
	void rotatePositionTagX(string direction, int x);
	void rotatePositionTagY(string direction, int y);
	void rotatePositionTagZ(string direction, int z);

	void printPositionTag();

	void rotateCube(mat4 rotateMatrix);
};