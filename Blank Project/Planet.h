#pragma once

#include "../nclgl/SceneNode.h"

class Planet : public SceneNode
{
public:
	Planet(Mesh* sphere, Vector3 modelScale, float boundingRadius, GLuint texture, const Matrix4& transform, bool orbitParent);
	~Planet(void) {}
	void Update(float dt) override;
protected:
	bool orbitParent;
};

