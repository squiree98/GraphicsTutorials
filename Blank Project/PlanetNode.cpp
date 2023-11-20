#include "PlanetNode.h"

PlanetNode::PlanetNode(Mesh* mesh, GLuint texture, Shader* newShader, Vector3 scale, Vector3 transform, Vector3 rotation, bool spin, float spinSpeed) {
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
	this->isSkinned = 0;
	this->spin = spin;
	this->spinSpeed = spinSpeed;
	this->rotation = rotation;
}

PlanetNode::~PlanetNode(void) {
	SceneNode::~SceneNode();
}

void PlanetNode::Draw(const OGLRenderer& r) {
	// set texture and shader
	SceneNode::Draw(r);
}

void PlanetNode::Update(float dt) {
	if (spin)
		SetTransform(GetTransform() * Matrix4::Rotation(spinSpeed * dt, rotation));

	SceneNode::Update(dt);
}