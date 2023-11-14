#include "../nclgl/Camera.h"
#include "../nclgl/Shader.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"

#include "Renderer.h"
#include "TerrainNode.h"
#include "PlanetNode.h"

#include <algorithm>

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	// set meshes up
	HeightMap* heightMap = new HeightMap(TEXTUREDIR"noise.png");
	heightMapSize = heightMap->GetHeightMapSize();
	Mesh* sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	// set textures up
	rockTexture = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTexture = SOIL_load_OGL_texture(TEXTUREDIR"grass.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	redPlanetTexture = SOIL_load_OGL_texture(TEXTUREDIR"red_planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	bumpMap = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!rockTexture || !grassTexture || !redPlanetTexture || !bumpMap)
		return;
	SetTextureRepeating(rockTexture, true);
	SetTextureRepeating(grassTexture, true);
	SetTextureRepeating(redPlanetTexture, true);
	SetTextureRepeating(bumpMap, true);

	// set shaders up
	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	//planetShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	planetShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	if (!terrainShader->LoadSuccess() || !planetShader->LoadSuccess())
		return;

	// set scene nodes up
	root = new SceneNode();
	terrainNode =		new TerrainNode(heightMap, grassTexture, rockTexture, terrainShader);
	planetNode =		new PlanetNode (sphere, redPlanetTexture, planetShader, Vector3(50,50,50), Vector3(3000,1000,3000));
	planetNodeMoon =	new PlanetNode (sphere, rockTexture, planetShader, Vector3(20, 20, 20), Vector3(100, 0, 0));
	root->AddChild(terrainNode);
	terrainNode->AddChild(planetNode);
	planetNode->AddChild(planetNodeMoon);

	// set the camera and lighting up
	camera = new Camera(-45.0f, 0.0f, Vector3(0.5f, 1.5f, 0.5f) * heightMapSize);
	light = new Light(heightMapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	// turn depth test on and start rendering
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

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.rbegin(), nodeList.rend(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* node) {
	if (node->GetMesh()) {
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

	SetShaderLight(*light);
}

void Renderer::DrawPlanets(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	// gett world transform of vertices not local transform
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);

	// set Texture up
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->GetTexture());

	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpMap);

	glUniform3fv(glGetUniformLocation(node->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	SetShaderLight(*light);
}