#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Light.h"

Renderer::Renderer(Window &parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	// create surface textures
	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	// create cube map
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
									TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg", 
									TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
									SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex)
		return;

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);

	// define shaders for reflection, skybox, and light
	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	skyboxShader =	new Shader("SkyBoxVertex.glsl",  "SkyBoxFragment.glsl");
	lightShader =	new Shader("PerPixelVertex.glsl",   "PerPixelFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess())
		return;

	// create light and camera with heightmap
	Vector3 heightmapSize = heightMap->GetHeightMapSize();

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1,1,1,1), heightmapSize.x);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width/(float)height, 45.0f);

	// enable depth test
	glEnable(GL_DEPTH_TEST);
	// enable blending for water
	glEnable(GL_BLEND);
	// set blending to standard linear interpolation
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// allow cube map to sample linearly between faces, prevents seams
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// set remaining values
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	delete lightShader;
	delete reflectShader;
	delete skyboxShader;
	delete heightMap;
	delete quad;
	delete light;
	delete camera;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 2.0f; // rotate 2 degrees per second
	waterCycle += dt * 0.25f; // water flows at 10 units a second
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	// order matters as water needs to be last as hight map colours are needed for blending
	DrawSkybox();
	DrawHeightMap();
	DrawWater();
}

void Renderer::DrawHeightMap() {
	//set shader and create light
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	// set texture and bump map for height map texture
	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	// reset model and texture matrices to default as water shader
	// will change them later (dont want them to affect height map)
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawSkybox() {
	// prevent skyboc from writing to depth buffer otherwise it will
	// fill the buffer and cause water and height map to be discarded
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	// draw quad. Shader will do rest
	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	// put water quad in middle of height map and rotate to be parallel
	Vector3 hSize = heightMap->GetHeightMapSize();
	// matrix will now be in center of height map, stretches it across the hieghtmap, and rotates it
	modelMatrix = Matrix4::Translation(hSize * 0.5f) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1,0,0));
	// water will now flow on x and z axis, will repeat 10 times over heightmap, and water will also rotate
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * 
					Matrix4::Scale(Vector3(10,10,10)) * 
					Matrix4::Rotation(90, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}