#include "../nclgl/Camera.h"
#include "../nclgl/Shader.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"

#include "Renderer.h"
#include "TerrainNode.h"
#include "PlanetNode.h"
#include "WaterNode.h"
#include "SkinnedNode.h"

#include <algorithm>

const int SHADOWSIZE = 2048;
int POSTPASSES = 0;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	SetUpMeshes();

	SetUpTextures();

	SetUpShaders();

	SetUpShadowMapping();

	SetUpPostProcessing();

	SetUpSceneHierarchies();


	ResetCameras();
	freeMovement = false;

	// turn depth test on and start rendering
	glEnable(GL_DEPTH_TEST);
	// needed to render water
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// allows us to sample linearly between cube map faces
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	init = true;
}

Renderer::~Renderer(void) {
	delete heightMap;

	delete activeCamera;
	for (Camera* x : cameraViews) {
		delete x;
	}

	delete quad;
	delete skyBoxQuad;
	delete waterQuad;

	delete light;

	delete terrainShader;
	delete planetShader;
	delete planetShaderShadows;
	delete waterShader;
	delete skyBoxShader;
	delete shadowShader;
	delete skinnedMeshShader;
	delete sceneShader;
	delete processShader;

	glDeleteTextures(1, &cubeMap);
	glDeleteTextures(1, &planetTexture1);
	glDeleteTextures(1, &planetTexture2);
	glDeleteTextures(1, &planetTexture3);
	glDeleteTextures(1, &rockTexture);
	glDeleteTextures(1, &redPlanetTexture);
	glDeleteTextures(1, &waterTexture);
	glDeleteTextures(1, &bumpMap);
	glDeleteTextures(1, &bufferFBO);
	glDeleteTextures(1, &processFBO);
	glDeleteTextures(1, &bufferColourTex[0]);
	glDeleteTextures(1, &bufferColourTex[1]);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &shadowFBO);
	glDeleteTextures(1, &shadowTex);

	delete root_1;
	delete terrainNode;
	delete floatingCube;
	delete orbitController;
	delete cubeMoon;
	delete cubeNode;
	delete rockNode1;
	delete rockNode2;
	delete rockNode3;
	delete waterNode;
	delete skinnedNode;

	delete root_2;
	delete mainPlanetNode;
	delete asteroid1;
	delete asteroid2;
	delete asteroid3;
	delete orbitController1;
	delete orbitController2;
	delete orbitController3;
	delete moon_1;
	delete orbitControllerMoon1;
	delete planet_2;
	delete planet_3;

	for (SceneNode* x : transparentNodeList) {
		delete x;
	}
	for (SceneNode* x : nodeList) {
		delete x;
	}
}

void Renderer::UpdateScene(float dt) {
	if (freeMovement) {
		activeCamera = cameraViews[cameraIndex];
		activeCamera->UpdateCamera(dt);
	}
	else {
		activeCamera = cameraViews[cameraIndex];
		float timePassed = activeCamera->AutoMoveCamera(dt);
		if (timePassed >= 9.0f)
			POSTPASSES = 10;
		if (timePassed >= 10.0f) {
			cameraIndex++;
			POSTPASSES = 0;
		}
		if (cameraIndex == 3)
			sceneView = 2;
		if (cameraIndex == 7)
			ResetCameras();
	}
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	waterNode->SetWaterRotate(dt, 2.0f);
	waterNode->SetWaterCycle(dt, 0.25f);

	switch (sceneView) {
	case (1):
		root_1->Update(dt);
		break;
	case(2):
		root_2->Update(dt);
		break;
	}
}

void Renderer::RenderScene() {
	// set up node lists for building
	switch (sceneView) {
	case (1):
		light = new Light(Vector3(0.0f, 4, 0.0f) * heightMapSize, Vector4(1, 1, 1, 1), heightMapSize.x * 15);
		BuildNodeLists(root_1);
		break;
	case(2):
		light = new Light(Vector3(3475.92, 593.262, 952.303), Vector4(1, 1, 1, 1), heightMapSize.x * 15);
		BuildNodeLists(root_2);
		break;
	}
	SortNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	DrawSkyBox();

	DrawShadowScene();

	// rebuild view and projection matrix for main scene
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	DrawNodes();

	if (sceneView == 1)
		DrawWater();

	ClearNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	DrawPostProcess();

	PresentScreen();
}

// methods for setting up scene

void Renderer::SetUpMeshes() {
	// set meshes up
	// height map for terrain
	heightMap = new HeightMap(TEXTUREDIR"noise.png");
	heightMapSize = heightMap->GetHeightMapSize();

	// sphere and quad for water, cubemap, and planets
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	cube = Mesh::LoadFromMeshFile("cube.msh");
	rock_1 = Mesh::LoadFromMeshFile("Rock_02.msh");
	rock_2 = Mesh::LoadFromMeshFile("Rock_05.msh");
	rock_3 = Mesh::LoadFromMeshFile("Rock_06.msh");
	waterQuad = Mesh::GenerateQuad();
	skyBoxQuad = Mesh::GenerateQuad();
	quad = Mesh::GenerateQuad();

	// load skinned mesh data
	skinnedMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	anim = new MeshAnimation("Role_T.anm");
	material = new MeshMaterial("Role_T.mat");
}

void Renderer::SetUpTextures() {
	// set textures up
	rockTexture = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetTexture1 = SOIL_load_OGL_texture(TEXTUREDIR"planet.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetTexture2 = SOIL_load_OGL_texture(TEXTUREDIR"planet_2.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetTexture3 = SOIL_load_OGL_texture(TEXTUREDIR"planet_3.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	redPlanetTexture = SOIL_load_OGL_texture(TEXTUREDIR"red_planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	waterTexture = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	bumpMap = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"right.png", TEXTUREDIR"left.png",
		TEXTUREDIR"top.png", TEXTUREDIR"bottom.png",
		TEXTUREDIR"front.png", TEXTUREDIR"back.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
	if (!rockTexture || !planetTexture1 || !planetTexture2 || !planetTexture3 || !redPlanetTexture || !waterTexture || !cubeMap || !bumpMap)
		return;
	SetTextureRepeating(rockTexture, true);
	SetTextureRepeating(planetTexture1, true);
	SetTextureRepeating(planetTexture2, true);
	SetTextureRepeating(planetTexture3, true);
	SetTextureRepeating(redPlanetTexture, true);
	SetTextureRepeating(waterTexture, true);
	SetTextureRepeating(bumpMap, true);
}

void Renderer::SetUpShaders() {
	// set shaders up
	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	planetShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	planetShaderShadows = new Shader("ShadowSceneVertex.glsl", "ShadowSceneFragment.glsl");
	waterShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	skinnedMeshShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");

	skyBoxShader = new Shader("SkyBoxVertex.glsl", "SkyBoxFragment.glsl");
	shadowShader = new Shader("ShadowVertex.glsl", "ShadowFragment.glsl");
	processShader = new Shader("TexturedVertex.glsl", "ProcessFragment.glsl");
	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!terrainShader->LoadSuccess() || !planetShader->LoadSuccess() || !planetShaderShadows->LoadSuccess() || !waterShader->LoadSuccess() || !skyBoxShader->LoadSuccess() || !shadowShader->LoadSuccess() || !skinnedMeshShader->LoadSuccess() || !processShader->LoadSuccess() || !sceneShader->LoadSuccess())
		return;
}

void Renderer::SetUpShadowMapping() {
	// set shadow texture up
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	// set shadow FBO up and attatch shadow texture
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SetUpPostProcessing() {
	// set post processing up
	// get texture for buffer
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 1920, 1080, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	for (int i = 0; i < 2; i++) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	// set buffer up
	glGenFramebuffers(1, &bufferFBO); // render scene here
	glGenFramebuffers(1, &processFBO);// post processing here

	// bind and attatch textures
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
	// check success
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0])
		return;
}

void Renderer::SetUpSceneHierarchies() {
	SetUpGroundScene();

	SetUpSpaceScene();
}

void Renderer::SetUpGroundScene() {
	// generate ground scene
	root_1 = new SceneNode();
	terrainNode = new TerrainNode(heightMap, planetTexture1, rockTexture, terrainShader);
	rockNode1 = new PlanetNode(rock_1, rockTexture, planetShaderShadows, Vector3(100, 100, 100), Vector3(0.2f, 0.8f, 0.75) * heightMapSize, Vector3(0, 0, 0), false, 0);
	rockNode2 = new PlanetNode(rock_2, rockTexture, planetShaderShadows, Vector3(130, 130, 130), Vector3(0.5f, 0.8f, 0.2f) * heightMapSize, Vector3(0, 0, 0), false, 0);
	rockNode3 = new PlanetNode(rock_3, rockTexture, planetShaderShadows, Vector3(200, 100, 200), Vector3(0.4f, 0.8f, 0.7f) * heightMapSize, Vector3(0, 0, 0), false, 0);
	floatingCube = new PlanetNode(cube, redPlanetTexture, planetShaderShadows, Vector3(200, 200, 200), Vector3(0.3f, 3.5f, 0.3f) * heightMapSize, Vector3(1, 1, 1), true, 30.0f);
	orbitController = new PlanetNode(NULL, NULL, NULL, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 1, 0), true, 40.0f);
	cubeMoon = new PlanetNode(cube, rockTexture, planetShaderShadows, Vector3(50, 50, 50), Vector3(300, 0, 0), Vector3(1, 1, 1), true, 45.0f);
	cubeNode = new PlanetNode(cube, rockTexture, planetShaderShadows, Vector3(500, 300, 500), Vector3(0.3f, 0.5f, 0.3f) * heightMapSize, Vector3(0, 0, 0), false, 0.0f);
	waterNode = new WaterNode(waterQuad, waterTexture, waterShader, terrainNode->GetModelScale());
	skinnedNode = new SkinnedNode(skinnedMesh, anim, material, skinnedMeshShader, Vector3(-50, 150, 100));
	root_1->AddChild(terrainNode);
	terrainNode->AddChild(rockNode1);
	terrainNode->AddChild(rockNode2);
	terrainNode->AddChild(rockNode3);
	terrainNode->AddChild(floatingCube);
	terrainNode->AddChild(cubeNode);
	cubeNode->AddChild(skinnedNode);
	floatingCube->AddChild(orbitController);
	orbitController->AddChild(cubeMoon);
}

void Renderer::SetUpSpaceScene() {
	root_2 = new SceneNode();
	mainPlanetNode = new PlanetNode(sphere, planetTexture1, planetShaderShadows, Vector3(800, 800, 800), Vector3(800, 0, 800), Vector3(0, 1, 0), true, 30.0f);
	asteroid1 = new PlanetNode(rock_1, rockTexture, planetShaderShadows, Vector3(50, 50, 50), Vector3(1500, 0, 0), Vector3(1, 1, 1), true, 20.0f);
	orbitController1 = new PlanetNode(NULL, NULL, NULL, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 1, 0), true, 40.0f);
	asteroid2 = new PlanetNode(rock_2, rockTexture, planetShaderShadows, Vector3(15, 15, 15), Vector3(1550, 0, 0), Vector3(1, 0, 1), true, 20.0f);
	orbitController2 = new PlanetNode(NULL, NULL, NULL, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1), true, -30.0f);
	asteroid3 = new PlanetNode(rock_3, rockTexture, planetShaderShadows, Vector3(40, 40, 40), Vector3(2000, 0, 0), Vector3(1, 1, 0), true, 20.0f);
	orbitController3 = new PlanetNode(NULL, NULL, NULL, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 1, 1), true, 60.0f);
	moon_1 = new PlanetNode(sphere, planetTexture3, planetShaderShadows, Vector3(250, 250, 250), Vector3(3500, 0, 0), Vector3(1, 1, 0), true, 20.0f);
	orbitControllerMoon1 = new PlanetNode(NULL, NULL, NULL, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 1, 0), true, 1.0f);
	planet_2 = new PlanetNode(sphere, planetTexture2, planetShader, Vector3(1200, 1200, 1200), Vector3(-7800, 1600, 7000), Vector3(1, 1, 1), true, 5.0f);
	planet_3 = new PlanetNode(sphere, redPlanetTexture, planetShader, Vector3(400, 400, 400), Vector3(-7000, 1100, -5400), Vector3(1, 1, 0), true, 60.0f);;
	root_2->AddChild(mainPlanetNode);
	mainPlanetNode->AddChild(orbitController1);
	orbitController1->AddChild(asteroid1);
	mainPlanetNode->AddChild(orbitController2);
	orbitController2->AddChild(asteroid2);
	mainPlanetNode->AddChild(orbitController3);
	orbitController3->AddChild(asteroid3);
	mainPlanetNode->AddChild(orbitControllerMoon1);
	orbitControllerMoon1->AddChild(moon_1);
	root_2->AddChild(planet_2);
	root_2->AddChild(planet_3);
}

// methods for scene hierarchy

void Renderer::BuildNodeLists(SceneNode* from) {
	Vector3 dir = from->GetWorldTransform().GetPositionVector() - activeCamera->GetPosition();
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
			node->Draw(*this);
			return;
		}
		if (node->GetIsSkinned() == 1) {
			glEnable(GL_CULL_FACE);
			DrawSkinned(node);
			node->Draw(*this);
			glDisable(GL_CULL_FACE);
			return;
		}
		if (node->GetIsHeightMap() == 0 && node->GetIsSkinned() == 0) {
			DrawPlanets(node);
			node->Draw(*this);
			return;
		}
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

	skyBoxQuad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawTerrain(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	// get world transform of vertices not local transform
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->GetRockTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "rockTex"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, node->GetPlanetTexture());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "planetTex"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bumpMap);
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "bumpTex"), 2);

	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "shadowTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(node->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	SetShaderLight(*light);
}

void Renderer::DrawPlanets(SceneNode* node) {
	BindShader(node->GetShader());
	UpdateShaderMatrices();

	// get world transform of vertices not local transform
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);

	// set Texture up
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->GetTexture());

	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpMap);

	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(node->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	SetShaderLight(*light);
}

void Renderer::DrawSkinned(SceneNode* node) {
	BindShader(node->GetShader());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "diffuseTex"), 0);

	UpdateShaderMatrices();
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);
}

void Renderer::DrawWater() {
	BindShader(waterNode->GetShader());

	glUniform3fv(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform1i(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterNode->GetTexture());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	// matrix will now be in center of height map, stretches it across the hieghtmap, and rotates it
	//modelMatrix = Matrix4::Translation(heightMapSize * 0.5f) * Matrix4::Scale(heightMapSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterNode->GetWaterCycle(), 0.0f, waterNode->GetWaterCycle())) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(90, Vector3(0, 0, 1));
	UpdateShaderMatrices();
	Matrix4 model = Matrix4::Translation(heightMapSize * 0.5f) * waterNode->GetTransform() * Matrix4::Scale(heightMapSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(waterNode->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);
	waterQuad->Draw();

	textureMatrix.ToIdentity();
}

// methods for shadowing

void Renderer::DrawShadowScene() {
	// set up gl for shadow map
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// generate shadow map
	BindShader(shadowShader);
	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0.2f, 0, 0.2f) * heightMapSize);
	projMatrix = Matrix4::Perspective(1, 15000, (float)width / (float)height, 90);
	shadowMatrix = projMatrix * viewMatrix;

	// draw nodes
	DrawShadowNodes();

	// undo above changes to set up gl for object generation
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
}

void Renderer::DrawShadowNodes() {
	for (const auto& i : nodeList) {
		DrawShadowNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawShadowNode(i);
	}
}

void Renderer::DrawShadowNode(SceneNode* node) {
	UpdateShaderMatrices();
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(shadowShader->GetProgram(), "modelMatrix"), 1, false, model.values);
	if (node->GetIsSkinned()) {
		node->SwitchShadowSkinned();
		node->Draw(*this);
		node->SwitchShadowSkinned();
	}
	else {
		node->Draw(*this);
	}
}

// methods for post processing

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	// apply post processing
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POSTPASSES; i++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();
		// swap colour buffers for second blur
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();
	}
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScreen() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
}

// camera switch

void Renderer::ChangeFreeMovement() {
	this->freeMovement = !freeMovement;
	ResetCameras();
}

void Renderer::ResetCameras() {
	cameraIndex = 0;
	sceneView = 1;
	cameraViews[0] = new Camera(0.0f, 45.0f, heightMapSize * Vector3(0.5f, 1.5f, 0.5f));
	cameraViews[1] = new Camera(-12.0f, 130.0f, Vector3(3070.0f, 870.0f, 100.0f));
	cameraViews[2] = new Camera(290.0f, -21.0f, Vector3(700.0f, 1100.0f, 3600.0f));
	cameraViews[3] = new Camera(-3.0f, 80.0f, Vector3(4360.0f, 230.0f, 135.0f));
	cameraViews[4] = new Camera(-17.0f, 330.0f, Vector3(-7470.0f, 4403.0f, 10258.0f));
	cameraViews[5] = new Camera(-35.0f, 85, Vector3(6720.0f, 6860.0f, 725.0f));
	POSTPASSES = 0;
}

void Renderer::ChangeScene() {
	if (freeMovement) {
		switch (sceneView) {
		case (2):
			sceneView = 1;
			break;
		case(1):
			sceneView = 2;
			break;
		}
	}
}