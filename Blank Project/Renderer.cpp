#include "Renderer.h"

#include "../nclgl/SceneNode.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"

#include <algorithm>

using namespace std;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	terrain = new HeightMap(TEXTUREDIR"noise.png");

	root = new SceneNode();

	Vector3 heightMapSize = terrain->GetHeightMapSize();
	camera = new Camera(-45.0f, 0.0f, heightMapSize * Vector3(0.5f, 5.0f, 0.5f));

	terrainTexture = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if(!terrainTexture)
		return;

	SetTextureRepeating(terrainTexture, true);

	terrainShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!terrainShader->LoadSuccess())
		return;

	SceneNode* terrainNode = new SceneNode();
	terrainNode->SetTransform(Matrix4::Translation(Vector3(0,0,0)));
	terrainNode->SetMesh(terrain);
	terrainNode->SetTexture(terrainTexture);

	root->AddChild(terrainNode);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	init = true;
}

Renderer::~Renderer(void)	{
	delete root;
	delete terrain;
	delete terrainShader;
	delete camera;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustrum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);
}

void Renderer::RenderScene()	{
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(terrainShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "diffuseTex"), 0);

	DrawNodes();

	ClearNodeLists();
}

void Renderer::BuildNodeLists(SceneNode* root) {
	//if (frameFrustrum.InsideFrustrum(*root)) {
		Vector3 dir = root->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		root->SetCameraDistance(Vector3::Dot(dir, dir));

		if (root->GetColour().w < 1.0f)
			transparentNodeList.push_back(root);
		else
			nodeList.push_back(root);

		for (vector<SceneNode*>::const_iterator i = root->GetChildIteratorStart(); i != root->GetChildIteratorEnd(); i++) {
			BuildNodeLists((*i));
		}
	//}
}

void Renderer::SortNodeLists() {
	sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	sort(nodeList.rbegin(), nodeList.rend(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList)
	{
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList)
	{
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* node) {
	if (node->GetMesh()) {
		Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());

		glUniformMatrix4fv(glGetUniformLocation(terrainShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(terrainShader->GetProgram(), "nodeColour"), 1, (float*)&node->GetColour());

		terrainTexture = node->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, terrainTexture);

		glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "useTexture"), terrainTexture);

		node->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}