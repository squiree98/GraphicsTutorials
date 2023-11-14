#pragma once

#include "../nclgl/SceneNode.h"

class WaterNode : public SceneNode
{
public:
	WaterNode(Mesh* mesh, GLuint texture, Shader* shader, Vector3 scale);
	~WaterNode(void);

	void	Update(float dt)			override;
	void	Draw(const OGLRenderer& r)	override;

	void SetWaterRotate(float dt, float value) { this->waterRotate += dt * value; }
	void SetWaterCycle(float dt, float value) { this->waterCycle += dt * value; }

	float GetWaterRotate() { return waterRotate; }
	float GetWaterCycle() { return waterCycle; }

	GLuint	GetPlanetTexture()			override { return NULL; }
	GLuint	GetRockTexture()			override { return NULL; }
protected:
	float waterRotate;
	float waterCycle;
};

