#include "../nclgl/Camera.h"
#include "../nclgl/Shader.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"

#include "Renderer.h"
#include "TerrainNode.h"
#include "PlanetNode.h"
#include "WaterNode.h"

#include <algorithm>

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	// set meshes up
	HeightMap* heightMap = new HeightMap(TEXTUREDIR"noise.png");
	heightMapSize = heightMap->GetHeightMapSize();
	Mesh* sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	quad = Mesh::GenerateQuad();

	// set textures up
	rockTexture			= SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetTexture		= SOIL_load_OGL_texture(TEXTUREDIR"images.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	redPlanetTexture	= SOIL_load_OGL_texture(TEXTUREDIR"red_planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	waterTexture		= SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	bumpMap				= SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap				= SOIL_load_OGL_cubemap(TEXTUREDIR"right.png",	TEXTUREDIR"left.png",
												TEXTUREDIR"top.png",		TEXTUREDIR"bottom.png",
												TEXTUREDIR"front.png",	TEXTUREDIR"back.png",
												SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
	
	if (!rockTexture || !planetTexture || !redPlanetTexture || !waterTexture || !cubeMap || !bumpMap)
		return;
	SetTextureRepeating(rockTexture, true);
	SetTextureRepeating(planetTexture, true);
	SetTextureRepeating(redPlanetTexture, true);
	SetTextureRepeating(waterTexture, true);
	SetTextureRepeating(bumpMap, true);

	// set shaders up
	terrainShader =  new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	planetShader =   new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	waterShader =	 new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	skyBoxShader =   new Shader("SkyBoxVertex.glsl", "SkyBoxFragment.glsl");
	if (!terrainShader->LoadSuccess() || !planetShader->LoadSuccess() || !waterShader->LoadSuccess() || !skyBoxShader->LoadSuccess())
		return;

	// set scene nodes up
	root = new SceneNode();
	terrainNode =		new TerrainNode(heightMap, planetTexture, rockTexture, terrainShader);
	planetNode =		new PlanetNode (sphere, redPlanetTexture, planetShader, Vector3(50,50,50), Vector3(3000,1000,3000));
	planetNodeMoon =	new PlanetNode (sphere, rockTexture, planetShader, Vector3(20, 20, 20), Vector3(100, 0, 0));
	waterNode =			new WaterNode(quad, waterTexture, waterShader, terrainNode->GetModelScale());
	root->AddChild(terrainNode);
	terrainNode->AddChild(planetNode);
	planetNode->AddChild(planetNodeMoon);

	// draw water node seperate as it must be drawn last
	// set the camera and lighting up
	camera = new Camera(-45.0f, 0.0f, Vector3(0.5f, 1.5f, 0.5f) * heightMapSize);
	light = new Light(heightMapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x * 2);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	// turn depth test on and start rendering
	glEnable(GL_DEPTH_TEST);
	// needed to render water
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// allows us to sample linearly between cube map faces
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

Renderer::~Renderer(void) {
	delete heightMap;

	delete quad;

	delete light;
	delete camera;

	delete terrainShader;
	delete planetShader;
	delete waterShader;
	delete skyBoxShader;

	delete root;
	delete terrainNode;
	delete planetNode;;
	delete planetNodeMoon;
	delete waterNode;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	waterNode->SetWaterRotate(dt, 2.0f);
	waterNode->SetWaterRotate(dt, 0.25f);

	root->Update(dt);
}

void Renderer::RenderScene() {
	// set up node lists for building
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSkyBox();

	DrawNodes();

	DrawWater();

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

void Renderer::DrawSkyBox() {
	glDepthMask(GL_FALSE);

	BindShader(skyBoxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawTerrain(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	// gett world transform of vertices not local transform
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);

	glUniform3fv(glGetUniformLocation(node->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->GetRockTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "rockTex"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, node->GetPlanetTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "grassTex"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bumpMap);
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "bumpTex"), 2);

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

void Renderer::DrawWater(){
	BindShader(waterNode->GetShader());

	glUniform3fv(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterNode->GetTexture());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	// matrix will now be in center of height map, stretches it across the hieghtmap, and rotates it
	modelMatrix = Matrix4::Translation(heightMapSize * 0.5f) * Matrix4::Scale(heightMapSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterNode->GetWaterCycle(), 0.0f, waterNode->GetWaterCycle())) *
					Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(90, Vector3(0, 0, 1));
	UpdateShaderMatrices();
	quad->Draw();
}