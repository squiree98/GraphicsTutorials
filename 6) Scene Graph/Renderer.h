#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/CubeRobot.h"

class Renderer : public OGLRenderer
{
public:
	Renderer(Window &parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;

protected:
	void DrawNode(SceneNode* node);

	SceneNode*	root_1;
	SceneNode*	root_2;
	Camera*		camera;
	Mesh*		cube;
	Shader*		shader;
};

