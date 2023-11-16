#include "SkinnedNode.h"

SkinnedNode::SkinnedNode(Mesh* mesh, MeshAnimation* anim, MeshMaterial* material, Shader* shader) {
	this->mesh = mesh;
	this->anim = anim;
	this->material = material;
	this->shader = shader;
	this->isSkinned = 1;
	this->isHeightMap = 0;
	this->modelScale = Vector3(0.5f, 0.5f, 0.5f);
	this->transform = Matrix4::Rotation(90, Vector3(1, 0, 0));

	// for every submesh get its respective texture
	for (int i = 0; i < mesh->GetSubMeshCount(); i++)
	{
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);
		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	currentFrame = 0;
	frameTime = 0.0f;
}

SkinnedNode::~SkinnedNode(void) {
	delete anim;
	delete mesh;
	delete material;
}

void SkinnedNode::Update(float dt) {
	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}

	SceneNode::Update(dt);
}

void SkinnedNode::Draw(const OGLRenderer& r) {
	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = mesh->GetInverseBindPose();
	const Matrix4* frameData = anim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < mesh->GetJointCount(); i++) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}
	int j = glGetUniformLocation(shader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < mesh->GetSubMeshCount(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, matTextures[i]);
		mesh->DrawSubMesh(i);
	}
}