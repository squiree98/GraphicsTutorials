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
private:
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

	// temp
	void DrawScene();
	vector<Mesh*> sceneMeshes;
	vector<Matrix4> sceneTransforms;
	float sceneTime;

	// camera free movement
	bool freeMovement;

	// height map and size of heightmap
	HeightMap* heightMap;
	Vector3 heightMapSize;

	Camera* camera;

	Mesh* quad;
	Mesh* skyBoxQuad;
	Mesh* waterQuad;

	// lighting
	Light* light;

	// shaders
	Shader* terrainShader;
	Shader* planetShader;
	Shader* waterShader;
	Shader* skyBoxShader;
	Shader* shadowShader;
	Shader* skinnedMeshShader;
	Shader* sceneShader;
	Shader* processShader;

	// textures + bump maps + cube map
	GLuint cubeMap;
	GLuint planetTexture;
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

	// variables for scene hierarchy
	SceneNode* root;
	TerrainNode* terrainNode;
	PlanetNode* planetNode;
	PlanetNode* planetNodeMoon;
	PlanetNode* cubeNode;
	WaterNode* waterNode;
	SkinnedNode* skinnedNode;


	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};

