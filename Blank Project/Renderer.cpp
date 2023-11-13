#include "../nclgl/Camera.h"
#include "../nclgl/Shader.h"
#include "../nclgl/HeightMap.h"

#include "Renderer.h"
#include "TerrainNode.h"
#include "PlanetNode.h"

#include <algorithm>

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	HeightMap* heightMap = new HeightMap(TEXTUREDIR"noise.png");
	Mesh* sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	rockTexture = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTexture = SOIL_load_OGL_texture(TEXTUREDIR"grass.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	if (!rockTexture || !grassTexture)
		return;
	SetTextureRepeating(rockTexture, true);
	SetTextureRepeating(grassTexture, true);

	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	planetShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	if (!terrainShader->LoadSuccess())
		return;

	root = new SceneNode();
	terrainNode = new TerrainNode(heightMap, grassTexture, rockTexture, terrainShader);
	planetNode = new PlanetNode(sphere, rockTexture, planetShader, Vector3(50,50,50), Vector3(3000,1000,3000));
	planetNodeMoon = new PlanetNode(sphere, grassTexture, planetShader, Vector3(20, 20, 20), Vector3(70, 0, 0));

	root->AddChild(terrainNode);
	terrainNode->AddChild(planetNode);
	planetNode->AddChild(planetNodeMoon);

	camera = new Camera(-45.0f, 0.0f, heightMap->GetHeightMapSize() * Vector3(0.5f, 1.0f, 0.5f));
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer::~Renderer(void) {
	// fill in later
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();

	root->Update(dt);
}

void Renderer::RenderScene() {
	// set up node lists for building
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawNodes();

	ClearNodeLists();
}

// methods for scene hierarchy

void Renderer::BuildNodeLists(SceneNode* from) {
	Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
	from->SetCameraDistance(Vector3::Dot(dir, dir));

	// add to transparent list or solid list
	if (from->GetColour().w < 1.0f)
		transparentNodeList.push_back(from);
	else
		nodeList.push_back(from);

	// iterate until all nodes are built
	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) {
		BuildNodeLists((*i));
	}
}

// sort the lists by how far they are from the camera
void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.rbegin(), nodeList.rend(), SceneNode::CompareByCameraDistance);
}

// go through each vector of nodes and draw them
void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

// draw node to screen
void Renderer::DrawNode(SceneNode* node) {
	if (node->GetMesh()) {
		Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());

		//BindShader(node->GetShader());

		glUniformMatrix4fv(glGetUniformLocation(terrainShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(terrainShader->GetProgram(), "nodeColour"), 1, (float*)&node->GetColour());
		
		GLuint texture;
		if (node->GetIsHeightMap() == 1) {
			DrawTerrain(node);
		}
		else {
			DrawPlanets(node);
		}

		// draw node
		node->Draw(*this);
	}
}

// clear both vectors once nodes have been drawn for next frame
void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

// methods used to draw objects

void Renderer::DrawTerrain(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->GetRockTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "rockTex"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, node->GetGrassTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "grassTex"), 1);
}

void Renderer::DrawPlanets(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	GLuint texture = node->GetTexture();

	glUniform4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "nodeColour"), 1, (float*)&node->GetColour());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "useTexture"), texture);
}