#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"

class Camera;
class Light;
class Shader;
class HeightMap;
class TerrainNode;
class PlanetNode;

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
	void DrawTerrain(SceneNode* node);
	void DrawPlanets(SceneNode* node);

private:
	HeightMap* heightMap;

	Camera* camera;

	Light* light;

	Shader* terrainShader;
	Shader* planetShader;

	Vector3 heightMapSize;

	GLuint grassTexture;
	GLuint rockTexture;
	GLuint redPlanetTexture;

	GLuint bumpMap;

	// variables for scene hierarchy
	SceneNode* root;
	TerrainNode* terrainNode;
	PlanetNode* planetNode;
	PlanetNode* planetNodeMoon;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};

