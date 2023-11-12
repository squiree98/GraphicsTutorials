#pragma once

#include "../NCLGL/OGLRenderer.h"
//#include "../nclgl/Frustrum.h"

class Camera;
class Shader;
class HeightMap;
class SceneNode;
class Planet;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);

	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;

protected:
	// build node graph hierarchy
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* node);

	// render scene
	SceneNode* CreateTerrain();
	Planet* CreatePlanet(SceneNode* parent, Vector3 scale, float boundingRadius, Vector3 transform, bool orbitParent);

	SceneNode* root;

	Shader*	   terrainShader;
	
	HeightMap* terrain;

	Camera*	   camera;

	GLuint	   rockTexture;
	GLuint	   grassTexture;

	//Frustrum frameFrustrum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
