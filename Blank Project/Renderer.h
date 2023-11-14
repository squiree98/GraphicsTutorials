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

class Renderer : public OGLRenderer
{
public:
	Renderer(Window &parent);
	~Renderer(void);

	void	UpdateScene(float dt)	override;
	void	RenderScene()			override;

	// methods for scene hierarchy
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* node);

	// methods used to draw terrain
	void DrawSkyBox();
	void DrawTerrain(SceneNode* node);
	void DrawPlanets(SceneNode* node);
	void DrawWater();

private:
	// height map and size of heightmap
	HeightMap* heightMap;
	Vector3 heightMapSize;

	Camera* camera;

	Mesh* quad;

	// lighting
	Light* light;

	// shaders
	Shader* terrainShader;
	Shader* planetShader;
	Shader* waterShader;
	Shader* skyBoxShader;

	// textures + bump maps = cube map
	GLuint cubeMap;
	GLuint planetTexture;
	GLuint rockTexture;
	GLuint redPlanetTexture;
	GLuint waterTexture;

	GLuint bumpMap;

	// variables for scene hierarchy
	SceneNode* root;
	TerrainNode* terrainNode;
	PlanetNode* planetNode;
	PlanetNode* planetNodeMoon;
	WaterNode* waterNode;


	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};

