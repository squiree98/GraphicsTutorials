#pragma once

#include "../nclgl/SceneNode.h"

class Terrain : public SceneNode
{
public:
	Terrain();
	~Terrain(void) {}

	void Update(float dt)	override;
	void BindShader()		override;
};

