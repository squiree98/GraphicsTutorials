#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"

class Camera;
class Light;
class Shader;
class HeightMap;
class TerrainNode;
class PlanetNode;
class WaterNode;
class SkinnedNode;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window &parent);
	~Renderer(void);

	void	UpdateScene(float dt)	override;
	void	RenderScene()			override;

	// camera method
	void ChangeFreeMovement();
	void ChangeScene();
private:
	// cameras
	void ResetCameras();

	// methods for scene hierarchy
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawShadowNodes();
	void DrawNode(SceneNode* node);
	void DrawShadowNode(SceneNode* node);

	// methods used to draw terrain
	void DrawSkyBox();
	void DrawShadowScene();
	void DrawTerrain(SceneNode* node);
	void DrawPlanets(SceneNode* node);
	void DrawSkinned(SceneNode* node);
	void DrawWater();

	// post processing methods
	void DrawPostProcess();
	void PresentScreen();

	// height map and size of heightmap
	HeightMap* heightMap;
	Vector3 heightMapSize;

	bool freeMovement;
	Camera* cameraViews[6];
	Camera* activeCamera;
	int cameraIndex;

	Mesh* quad;
	Mesh* skyBoxQuad;
	Mesh* waterQuad;

	// lighting
	Light* light;

	// shaders
	Shader* terrainShader;
	Shader* planetShader;
	Shader* planetShaderShadows;
	Shader* waterShader;
	Shader* skyBoxShader;
	Shader* shadowShader;
	Shader* skinnedMeshShader;
	Shader* sceneShader;
	Shader* processShader;

	// textures + bump maps + cube map
	GLuint cubeMap;
	GLuint planetTexture1;
	GLuint planetTexture2;
	GLuint planetTexture3;
	GLuint rockTexture;
	GLuint redPlanetTexture;
	GLuint waterTexture;
	GLuint bumpMap;
	// post processing
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;
	// shadow mapping
	GLuint shadowFBO;
	GLuint shadowTex;

	int sceneView;

	// variables for scene hierarchy
	SceneNode* root_1;
	TerrainNode* terrainNode;
	PlanetNode* floatingCube;
	PlanetNode* orbitController;
	PlanetNode* cubeMoon;
	PlanetNode* cubeNode;
	PlanetNode* rockNode1;
	PlanetNode* rockNode2;
	PlanetNode* rockNode3;
	WaterNode* waterNode;
	SkinnedNode* skinnedNode;

	SceneNode* root_2;
	PlanetNode* mainPlanetNode;
	PlanetNode* asteroid1;
	PlanetNode* orbitController1;
	PlanetNode* asteroid2;
	PlanetNode* orbitController2;
	PlanetNode* asteroid3;
	PlanetNode* orbitController3;
	PlanetNode* moon_1;
	PlanetNode* orbitControllerMoon1;

	PlanetNode* planet_2;
	PlanetNode* planet_3;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};

