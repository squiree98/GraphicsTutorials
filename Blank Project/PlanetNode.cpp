#include "PlanetNode.h"

PlanetNode::PlanetNode(Mesh* mesh, GLuint texture, Shader* newShader, Vector3 scale, Vector3 transform) {
	this->mesh = mesh;
	this->colour = Vector4(1, 1, 1, 1);
	this->parent = NULL;
	this->modelScale = scale;
	this->shader = newShader;
	this->boundingRadius = 50.0f;
	this->distanceFromCamera = 0.0f;
	this->texture = texture;
	this->transform = Matrix4::Translation(transform);
	this->isHeightMap = 0;
}

void PlanetNode::Draw(const OGLRenderer& r) {
	// set texture and shader
	SceneNode::Draw(r);
}

void PlanetNode::Update(float dt) {
	SetTransform(GetTransform() * Matrix4::Rotation(30.0f * dt, Vector3(0, 1, 0)));

	SceneNode::Update(dt);
}