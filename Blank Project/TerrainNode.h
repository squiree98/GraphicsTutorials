#pragma once

#include "../nclgl/SceneNode.h"
#include "../nclgl/HeightMap.h"

class TerrainNode : public SceneNode
{
public:
	TerrainNode(HeightMap* heightMap, GLuint planetTexture, GLuint rockTexture, Shader* shader);
	~TerrainNode(void);

	void Update(float dt) override;
	void Draw(const OGLRenderer& r) override;

	GLuint GetPlanetTexture() override { return planetTexture; }
	GLuint GetRockTexture() override { return rockTexture; }
protected:
	GLuint rockTexture;
	GLuint planetTexture;
};

