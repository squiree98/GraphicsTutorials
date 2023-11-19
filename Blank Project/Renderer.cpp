#include "../nclgl/Camera.h"
#include "../nclgl/Shader.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

#include "Renderer.h"
#include "TerrainNode.h"
#include "PlanetNode.h"
#include "WaterNode.h"
#include "SkinnedNode.h"

#include <algorithm>

const int SHADOWSIZE = 2048;
const int POSTPASSES = 0;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	// set meshes up
	// height map for terrain
	heightMap = new HeightMap(TEXTUREDIR"noise.png");
	heightMapSize = heightMap->GetHeightMapSize();

	// sphere and quad for water, cubemap, and planets
	Mesh* sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	Mesh* cube	 = Mesh::LoadFromMeshFile("Cube.msh");
	waterQuad = Mesh::GenerateQuad();
	skyBoxQuad = Mesh::GenerateQuad();
	quad = Mesh::GenerateQuad();

	// load skinned mesh data
	Mesh* skinnedMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	MeshAnimation* anim = new MeshAnimation("Role_T.anm");
	MeshMaterial* material = new MeshMaterial("Role_T.mat");

	// set textures up
	rockTexture			= SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetTexture		= SOIL_load_OGL_texture(TEXTUREDIR"images_1.jpeg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
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
	terrainShader =		new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	planetShader =		new Shader("ShadowSceneVertex.glsl", "ShadowSceneFragment.glsl");
	waterShader =		new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	skyBoxShader =		new Shader("SkyBoxVertex.glsl", "SkyBoxFragment.glsl");
	shadowShader =		new Shader("ShadowVertex.glsl", "ShadowFragment.glsl");
	skinnedMeshShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	processShader =		new Shader("TexturedVertex.glsl", "ProcessFragment.glsl");
	sceneShader =		new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!terrainShader->LoadSuccess() || !planetShader->LoadSuccess() || !waterShader->LoadSuccess() || !skyBoxShader->LoadSuccess() || !shadowShader->LoadSuccess() || !skinnedMeshShader->LoadSuccess() || !processShader->LoadSuccess() || !sceneShader->LoadSuccess())
		return;

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

	// set post processing up
	// get texture for buffer
	glGenTextures	(1, &bufferDepthTex);
	glBindTexture	(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D	(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	for (int i = 0; i < 2; i++) {
		glGenTextures	(1, &bufferColourTex[i]);
		glBindTexture	(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D	(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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

	// temp
	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));
	// store meshes model matrices here
	sceneTransforms.resize(4);
	// set model matrix for terrain as it wont move
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTime = 0.0f;

	// set scene nodes up
	root = new SceneNode();
	terrainNode =		new TerrainNode(heightMap, planetTexture, rockTexture, terrainShader);
	planetNode =		new PlanetNode (sphere, redPlanetTexture, planetShader, Vector3(50,50,50), Vector3(3000,1000,3000), true);
	planetNodeMoon =	new PlanetNode (sphere, rockTexture, planetShader, Vector3(20, 20, 20), Vector3(100, 0, 0), true);
	cubeNode =			new PlanetNode(cube, rockTexture, planetShader, Vector3(500, 300, 500), Vector3(0.3f, 0.5f, 0.3f) * heightMapSize, false);
	waterNode =			new WaterNode(waterQuad, waterTexture, waterShader, terrainNode->GetModelScale());
	skinnedNode =		new SkinnedNode(skinnedMesh, anim, material, skinnedMeshShader, Vector3(-50, 150, 100));
	root->AddChild(terrainNode);
	terrainNode->AddChild(planetNode);
	terrainNode->AddChild(cubeNode);
	//cubeNode->AddChild(skinnedNode);
	planetNode->AddChild(planetNodeMoon);

	Vector3 temp = Vector3(1, 2, 1) * heightMapSize;
	// draw water node seperate as it must be drawn last
	// set the camera and lighting up
	camera = new Camera(0.0f, 45.0f, temp);
	//camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	freeMovement = true;
	light = new Light(temp, Vector4(1, 1, 1, 1), heightMapSize.x * 2);
	//light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

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
	sceneTime += dt;
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	for (int i = 1; i < 4; i++)
	{
		Vector3 t = Vector3(-10 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));
	}

	waterNode->SetWaterRotate(dt, 2.0f);
	waterNode->SetWaterCycle(dt, 0.25f);

	root->Update(dt);
}

void Renderer::RenderScene() {
	// set up node lists for building
	BuildNodeLists(root);
	SortNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	DrawSkyBox();

	// DrawShadowScene();

	// rebuild view and projection matrix for main scene
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	//DrawScene();

	DrawNodes();

	//DrawWater();

	ClearNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	DrawPostProcess();

	PresentScreen();
}

// temp
void Renderer::DrawScene() {
	BindShader(planetShader);
	SetShaderLight(*light);
	// set up view matrix to be back in cameras viewpoint not lights
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	// set uniforms for shaders
	glUniform1i(glGetUniformLocation(planetShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(planetShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(planetShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(planetShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rockTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpMap);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (int i = 0; i < 4; i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
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

	/*glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);*/

	glUniform3fv(glGetUniformLocation(node->GetShader()->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	SetShaderLight(*light);
}

void Renderer::DrawSkinned(SceneNode* node) {
	BindShader(node->GetShader());
	glUniform1i(glGetUniformLocation(node->GetShader()->GetProgram(), "diffuseTex"), 0);

	//modelMatrix = modelMatrix * Matrix4::Rotation(90, Vector3(-1,0,0)) * Matrix4::Scale(node->GetModelScale()) * node->GetWorldTransform();
	UpdateShaderMatrices();
	Matrix4 model = node->GetWorldTransform() * Matrix4::Scale(node->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(node->GetShader()->GetProgram(), "modelMatrix"), 1, false, model.values);
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
	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 150000, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	//// render shadow scene
	//for (int i = 0; i < 4; i++)
	//{
	//	modelMatrix = sceneTransforms[i];
	//	UpdateShaderMatrices();
	//	sceneMeshes[i]->Draw();
	//}

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
	node->Draw(*this);
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0],0);
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

// camera resetter

void Renderer::ChangeFreeMovement() {
	this->freeMovement = !freeMovement;
	camera->SetPosition(Vector3(0.5f, 1.5f, 0.5f) * heightMapSize);
	camera->SetPitch(0.0f);
	camera->SetYaw(45.0f);
	camera->ResetScene();
}