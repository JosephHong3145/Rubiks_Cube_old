#include "Sphere.h"
Sphere::Sphere() {

}

Sphere::Sphere(vec3 basePos, float rad, float poly, Shader S){
	currentShader = S;
	basePosition = basePos;
	polyCount = poly;
	radius = rad;
	this->update();
}

//update the sphere 
void Sphere::update() {

	modelMatrix = rotate(mat4(1.0f), radians(baseRotation), vec3(0.0f, 0.0f, 1.0f)) * scale(mat4(1.0f), vec3(radius) * currentScaleFactor);

	modelMatrix = translate(mat4(1.0f), (basePosition + centering) * currentScaleFactor) * modelMatrix;

	//setting custom rotation
	modelMatrix = customRotation * modelMatrix;

	//setting custom translation 
	modelMatrix = customTranslation * modelMatrix;
}

void Sphere::setDefaultRadius(float rad) {
	radius = rad;
	this->update();
}

void Sphere::setDefaultRotation(float angle) {
	baseRotation = angle;
	this->update();
}

void Sphere::setDefaultPosition(vec3 position) {
	basePosition = position;
	this->update();
}

void Sphere::setCustomScaling(float scaleFactor) {
	currentScaleFactor = scaleFactor;
}

void Sphere::setCustomRotation(mat4 rotationMatrix) {
	customRotation = rotationMatrix;
}

void Sphere::setCustomTranslation(vec3 offSet) {
	customTranslation = translate(mat4(1.0f), offSet);
}

void Sphere::rotateWithWorld(mat4 rotateX, mat4 rotateY) {
	modelMatrix = rotateY * rotateX * modelMatrix;
}

void Sphere::setCurrentShader(Shader S) {
	currentShader = S;
	this->update();
}

void Sphere::drawModel() {
	currentShader.setMat4("worldMatrix", modelMatrix);
	glDrawArrays(GL_LINES, 0, polyCount);
}