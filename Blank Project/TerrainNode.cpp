#include "TerrainNode.h"

TerrainNode::TerrainNode(HeightMap* heightMap, GLuint givenGrassTexture, GLuint givenRockTexture, Shader* newShader) {
	this->mesh = heightMap;
	this->colour = Vector4(1,1,1,1);
	this->parent = NULL;
	this->modelScale = Vector3(1, 1, 1);
	this->shader = newShader;
	this->boundingRadius = 0.0f;
	this->distanceFromCamera = 0.0f;
	this->grassTexture = givenGrassTexture;
	this->rockTexture = givenRockTexture;
	this->isHeightMap = 1;
	// shouldn't be used but if needed
	this->texture = givenGrassTexture;
}

void TerrainNode::Draw(const OGLRenderer& r) {
	// set texture and shader
	SceneNode::Draw(r);
}

void TerrainNode::Update(float dt) {
	SceneNode::Update(dt);
}