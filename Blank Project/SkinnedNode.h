#pragma once

#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

class SkinnedNode : public SceneNode
{
public:
	SkinnedNode(Mesh* mesh, MeshAnimation* anim, MeshMaterial* material, Shader* shader, Vector3 transform);
	~SkinnedNode(void);

	void Update(float dt)			override;
	void Draw(const OGLRenderer& r) override;

protected:
	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;

	int currentFrame;
	float frameTime;

	int framesWalking;
};

