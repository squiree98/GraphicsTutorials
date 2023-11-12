#include "Renderer.h"

#include "../nclgl/SceneNode.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "Planet.h"

#include <algorithm>

using namespace std;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	terrain = new HeightMap(TEXTUREDIR"noise.png");

	root = new SceneNode();

	Vector3 heightMapSize = terrain->GetHeightMapSize();
	camera = new Camera(-45.0f, 0.0f, heightMapSize * Vector3(0.5f, 1.0f, 0.5f));

	SetTextureRepeating(rockTexture, true);
	SetTextureRepeating(grassTexture, true);

	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	if (!terrainShader->LoadSuccess())
		return;

	rockTexture =  SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTexture = SOIL_load_OGL_texture(TEXTUREDIR"grass.jpg",       SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


	if (!rockTexture || !grassTexture)
		return;

	SceneNode*	terrainNode    = CreateTerrain();
	Planet*		planetNode	   = CreatePlanet(terrainNode, Vector3(50,50,50), 50.0f, Vector3(3000, 1000, 3000), false);
	Planet*		planetMoonNode = CreatePlanet(planetNode, Vector3(20,20,20), 20.0f, Vector3(-100, 0, -100), true);


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
	//frameFrustrum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);
}

void Renderer::RenderScene()	{
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(terrainShader);

	glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "rockTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rockTexture);

	glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "grassTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, grassTexture);

	UpdateShaderMatrices();

	DrawNodes();

	ClearNodeLists();
}

// Methods dealing with node hierarchy

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

		GLuint nodeTexture = node->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, nodeTexture);

		glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "useTexture"), nodeTexture);

		node->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

// Methods that render scene

SceneNode* Renderer::CreateTerrain() {
	SceneNode* terrainNode = new SceneNode();
	terrainNode->SetTransform(Matrix4::Translation(Vector3(0, 0, 0)));
	terrainNode->SetMesh(terrain);
	terrainNode->SetTexture(rockTexture);

	root->AddChild(terrainNode);

	return terrainNode;
}

Planet* Renderer::CreatePlanet(SceneNode* parent, Vector3 scale, float boundingRadius, Vector3 transform, bool orbitParent) {
	Mesh* sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	Planet* planetNode = new Planet(sphere, scale, boundingRadius, rockTexture, Matrix4::Translation(transform), orbitParent);

	parent->AddChild(planetNode);

	return planetNode;
}