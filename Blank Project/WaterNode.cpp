#include "WaterNode.h"

WaterNode::WaterNode(Mesh* mesh, GLuint texture, Shader* shader, Vector3 scale) {
	this->mesh = mesh;
	this->colour = Vector4(1, 1, 1, 1);
	this->parent = NULL;
	this->modelScale = scale;
	this->shader = shader;
	this->boundingRadius = 50.0f;
	this->distanceFromCamera = 0.0f;
	this->texture = texture;
	this->transform = Matrix4::Translation(Vector3(0,-20,0));
	this->isHeightMap = 0;
	this->isSkinned = 0;
	waterRotate = 0.0f;
	waterCycle = 0.0f;
}

WaterNode::~WaterNode(void) {
	for (unsigned int i = 0; i < children.size(); ++i) {
		delete children[1];
	}
}

void WaterNode::Draw(const OGLRenderer& r) {
	SceneNode::Draw(r);
}

void WaterNode::Update(float dt) {
	SceneNode::Update(dt);
}