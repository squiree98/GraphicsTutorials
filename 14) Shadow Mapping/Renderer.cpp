#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"

float SHADOWSIZE = 2048.0f;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	// camera and light
	camera = new Camera(-30.0f, 315.0f, Vector3(0,0,0));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1,1,1,1), 250.0f);

	//// shaders
	sceneShader = new  Shader("ShadowSceneVertex.glsl", "ShadowSceneFragment.glsl");
	shadowShader = new Shader("ShadowVertex.glsl", "ShadowFragment.glsl");

	if (!sceneShader->LoadSuccess() || !shadowShader->LoadSuccess())
		return;

	// define depth buffer and bind shadow texture
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// create Frame Buffer Object
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// create meshes
	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

	// texture for terrain
	sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	SetTextureRepeating(sceneDiffuse, true);
	SetTextureRepeating(sceneBump, true);
	glEnable(GL_DEPTH_TEST);

	// store meshes model matrices here
	sceneTransforms.resize(4);
	// set model matrix for terrain as it wont move
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1,0,0)) * Matrix4::Scale(Vector3(10,10,1));
	sceneTime = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

	for (auto i : sceneMeshes) {
		delete i;
	}

	delete camera;
	delete sceneShader;
	delete shadowShader;
}

void Renderer::UpdateScene(float dt) {
	// count every frame
	camera->UpdateCamera(dt);
	sceneTime += dt;

	for (int  i = 1; i < 4; i++)
	{
		Vector3 t = Vector3(-10 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1,0,0));
	}
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawShadowScene();
	DrawMainScene();
}

void Renderer::DrawShadowScene() {
	// generate depth buffer image from the light pov
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	// clear whatever was in buffer ready for next buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	// increase virtual window size as shadow map depth buffer is larger than screen
	glViewport(0,0, SHADOWSIZE, SHADOWSIZE);
	// disable all colours as they are not needed for shadow buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	// bind shader
	BindShader(shadowShader);

	// build view matrix from lights view point
	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0,0,0));
	projMatrix = Matrix4::Perspective(1,100,1,45);
	shadowMatrix = projMatrix * viewMatrix;

	// render shadow scene
	for (int i = 0; i < 4; i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	// re-enable colours, shrink screen back to original size, and return to original buffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0,0,width,height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// much like before with new parts
void Renderer::DrawMainScene() {
	BindShader(sceneShader);
	SetShaderLight(*light);
	// set up view matrix to be back in cameras viewpoint not lights
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width/(float)height, 45.0f);

	// set uniforms for shaders
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sceneBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (int i = 0; i < 4; i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
}