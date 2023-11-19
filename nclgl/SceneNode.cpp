#include "SceneNode.h"

SceneNode::SceneNode(Mesh* mesh, Vector4 colour) {
	this->mesh = mesh;
	this->colour = colour;
	this->isHeightMap = 0;
	this->isSkinned = 0;
	parent = NULL;
	modelScale = Vector3(1, 1, 1);
	shader = NULL;
	boundingRadius = 1.0f;
	distanceFromCamera = 0.0f;
	texture = 0;
}

SceneNode::~SceneNode(void) {
	for (unsigned int i = 0; i < children.size(); ++i) {
		delete children[1];
	}

	delete shader;
	glDeleteTextures(1, &texture);
}

void SceneNode::AddChild(SceneNode* newChild) {
	children.push_back(newChild);
	newChild->parent = this;
}

void SceneNode::Draw(const OGLRenderer &r) {
	if (mesh)
		mesh->Draw();
}

void SceneNode::Update(float dt) {
	if (parent) { 
		worldTransform = parent->worldTransform * transform;
	}
	else { worldTransform = transform; }

	for (vector<SceneNode*>::iterator i = children.begin(); i != children.end(); ++i)
	{
		(*i)->Update(dt);
	}
}

// ignore
GLuint SceneNode::GetPlanetTexture() {
	// to be overriden
	return NULL;
}
//ignore
GLuint SceneNode::GetRockTexture() {
	// to be overriden
	return NULL;
}