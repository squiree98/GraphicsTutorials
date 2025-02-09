#pragma once

#include "../nclgl/SceneNode.h"

class PlanetNode : public SceneNode
{
public:
	PlanetNode(Mesh* mesh, GLuint texture, Shader* newShader, Vector3 scale, Vector3 transform, Vector3 rotation, bool spin, float spinSpeed);
	~PlanetNode(void);

	void	Update(float dt)			override;
	void	Draw(const OGLRenderer& r)	override;

	GLuint	GetPlanetTexture()			override { return NULL; }
	GLuint	GetRockTexture()			override { return NULL; }

protected:
	bool spin;
	float spinSpeed;

	Vector3 rotation;
};

