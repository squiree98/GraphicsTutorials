#include "TerrainNode.h"

TerrainNode::TerrainNode(HeightMap* heightMap, GLuint givenPlanetTexture, GLuint givenRockTexture, Shader* newShader) {
	this->mesh = heightMap;
	this->colour = Vector4(1,1,1,1);
	this->parent = NULL;
	this->modelScale = Vector3(1, 1, 1);
	this->shader = newShader;
	this->boundingRadius = 0.0f;
	this->distanceFromCamera = 0.0f;
	this->planetTexture = givenPlanetTexture;
	this->rockTexture = givenRockTexture;
	this->isHeightMap = 1;
	this->isSkinned = 0;
	// shouldn't be used but if needed
	this->texture = givenRockTexture;
}

TerrainNode::~TerrainNode(void) {
	for (unsigned int i = 0; i < children.size(); ++i) {
		delete children[1];
	}
}

void TerrainNode::Draw(const OGLRenderer& r) {
	// set texture and shader
	SceneNode::Draw(r);
}

void TerrainNode::Update(float dt) {
	SceneNode::Update(dt);
}