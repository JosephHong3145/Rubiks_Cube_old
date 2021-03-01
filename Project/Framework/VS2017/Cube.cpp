#include "Cube.h"
Cube::Cube() {

}

Cube::Cube(vec3 basePos, Shader S) {
	//setting base scaling, translation and rotation
	currentShader = S;
	basePosition = basePos;
	this->update();
}

//update the cube 
void Cube::update() {
	mat4 scaleMatrix = scale(mat4(1.0f), size * currentScaleFactor);
	mat4 translateMatrix = translate(mat4(1.0f), (positionTag + basePosition)*currentScaleFactor);

	modelMatrix = translateMatrix * baseRotationMatrix * scaleMatrix;
}

void Cube::setDefaultSize(vec3 s) {
	size = s;
	this->update();
}

void Cube::setDefaultRotation(mat4 rotationMatrix) {
	baseRotationMatrix = rotationMatrix;
	this->update();
}

void Cube::setDefaultPosition(vec3 position) {
	basePosition = position;
	this->update();
}

void Cube::setCustomScaling(float scaleFactor) {
	currentScaleFactor = scaleFactor;
	this->update();
}

void Cube::rotateCube(mat4 rotateMatrix) {
	baseRotationMatrix = rotateMatrix * baseRotationMatrix;
}

void Cube::setCustomTranslation(vec3 offSet) {
	customTranslation = translate(mat4(1.0f), offSet);
}

void Cube::drawModel() {
	this->drawBotFace();
	this->drawLeftFace();
	this->drawRightFace();
	this->drawFrontFace();
	this->drawBackFace();
	this->drawTopFace();
}

void Cube::drawLeftFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, 6); // left face
}

void Cube::drawBackFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 6, 6); // back face
}

void Cube::drawBotFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 12, 6); // bottom face
}

void Cube::drawFrontFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 18, 6); // front face
}

void Cube::drawRightFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 24, 6); // right face
}

void Cube::drawTopFace() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_TRIANGLES, 30, 6); // top face
}


void Cube::setCurrentShader(Shader S) {
	currentShader = S;
	this->update();
}

void Cube::setDepthMap(mat4 lightProjMatrix, mat4 lightViewMatrix) {
	//currentShader.setVec3("position", vec3(0.0f, 1.0f, 30.0f));
	currentShader.setMat4("transform_in_light_space", lightProjMatrix * lightViewMatrix * modelMatrix);
	this->update();
}

//the following is specifically for rubik's cube: 
void Cube::setPositionTag(vec3 tag) {
	positionTag = tag;
}

vec3 Cube::getPositionTag() { return positionTag; }

void Cube::setInitialPositionTag(vec3 tag) {
	initialPositionTag = tag;
}

vec3 Cube::getInitialPositionTag() { return initialPositionTag; }

void Cube::rotatePositionTagX(string direction, int x) {
	if (direction == "CCW") {
		if (positionTag == vec3(x, 0, 0)) { positionTag = vec3(x, 2, 0); }
		else if (positionTag == vec3(x, 2, 0)) { positionTag = vec3(x, 2, 2); }
		else if (positionTag == vec3(x, 2, 2)) { positionTag = vec3(x, 0, 2); }
		else if (positionTag == vec3(x, 0, 2)) { positionTag = vec3(x, 0, 0); }

		else if (positionTag == vec3(x, 1, 0)) { positionTag = vec3(x, 2, 1); }
		else if (positionTag == vec3(x, 2, 1)) { positionTag = vec3(x, 1, 2); }
		else if (positionTag == vec3(x, 1, 2)) { positionTag = vec3(x, 0, 1); }
		else if (positionTag == vec3(x, 0, 1)) { positionTag = vec3(x, 1, 0); }
	}

	else if (direction == "CW") {
		if (positionTag == vec3(x, 0, 0)) { positionTag = vec3(x, 0, 2); }
		else if (positionTag == vec3(x, 0, 2)) { positionTag = vec3(x, 2, 2); }
		else if (positionTag == vec3(x, 2, 2)) { positionTag = vec3(x, 2, 0); }
		else if (positionTag == vec3(x, 2, 0)) { positionTag = vec3(x, 0, 0); }

		else if (positionTag == vec3(x, 1, 0)) { positionTag = vec3(x, 0, 1); }
		else if (positionTag == vec3(x, 0, 1)) { positionTag = vec3(x, 1, 2); }
		else if (positionTag == vec3(x, 1, 2)) { positionTag = vec3(x, 2, 1); }
		else if (positionTag == vec3(x, 2, 1)) { positionTag = vec3(x, 1, 0); }
	}
}

void Cube::rotatePositionTagY(string direction, int y) {
	if (direction == "CCW") {
		if (positionTag == vec3(0, y, 0)) { positionTag = vec3(2, y, 0); }
		else if (positionTag == vec3(2, y, 0)) { positionTag = vec3(2, y, 2); }
		else if (positionTag == vec3(2, y, 2)) { positionTag = vec3(0, y, 2); }
		else if (positionTag == vec3(0, y, 2)) { positionTag = vec3(0, y, 0); }

		else if (positionTag == vec3(1, y, 0)) { positionTag = vec3(2, y, 1); }
		else if (positionTag == vec3(2, y, 1)) { positionTag = vec3(1, y, 2); }
		else if (positionTag == vec3(1, y, 2)) { positionTag = vec3(0, y, 1); }
		else if (positionTag == vec3(0, y, 1)) { positionTag = vec3(1, y, 0); }
	}

	else if (direction == "CW") {
		if (positionTag == vec3(0, y, 0)) { positionTag = vec3(0, y, 2); }
		else if (positionTag == vec3(0, y, 2)) { positionTag = vec3(2, y, 2); }
		else if (positionTag == vec3(2, y, 2)) { positionTag = vec3(2, y, 0); }
		else if (positionTag == vec3(2, y, 0)) { positionTag = vec3(0, y, 0); }

		else if (positionTag == vec3(1, y, 0)) { positionTag = vec3(0, y, 1); }
		else if (positionTag == vec3(0, y, 1)) { positionTag = vec3(1, y, 2); }
		else if (positionTag == vec3(1, y, 2)) { positionTag = vec3(2, y, 1); }
		else if (positionTag == vec3(2, y, 1)) { positionTag = vec3(1, y, 0); }
	}
}

void Cube::rotatePositionTagZ(string direction, int z) {
	if (direction == "CCW") {
		if (positionTag == vec3(0, 0, z)) { positionTag = vec3(2, 0, z); }
		else if (positionTag == vec3(2, 0, z)) { positionTag = vec3(2, 2, z); }
		else if (positionTag == vec3(2, 2, z)) { positionTag = vec3(0, 2, z); }
		else if (positionTag == vec3(0, 2, z)) { positionTag = vec3(0, 0, z); }

		else if (positionTag == vec3(1, 0, z)) { positionTag = vec3(2, 1, z); }
		else if (positionTag == vec3(2, 1, z)) { positionTag = vec3(1, 2, z); }
		else if (positionTag == vec3(1, 2, z)) { positionTag = vec3(0, 1, z); }
		else if (positionTag == vec3(0, 1, z)) { positionTag = vec3(1, 0, z); }
	}

	else if (direction == "CW") {

		if (positionTag == vec3(0, 0, z)) { positionTag = vec3(0, 2, z); }
		else if (positionTag == vec3(0, 2, z)) { positionTag = vec3(2, 2, z); }
		else if (positionTag == vec3(2, 2, z)) { positionTag = vec3(2, 0, z); }
		else if (positionTag == vec3(2, 0, z)) { positionTag = vec3(0, 0, z); }

		else if (positionTag == vec3(1, 0, z)) { positionTag = vec3(0, 1, z); }
		else if (positionTag == vec3(0, 1, z)) { positionTag = vec3(1, 2, z); }
		else if (positionTag == vec3(1, 2, z)) { positionTag = vec3(2, 1, z); }
		else if (positionTag == vec3(2, 1, z)) { positionTag = vec3(1, 0, z); }
	}
}

void Cube::printPositionTag() {
	cout << positionTag.x << ", " << positionTag.y << ", " << positionTag.z << endl;
}