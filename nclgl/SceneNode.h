#pragma once

#include "Matrix4.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Mesh.h"
#include <vector>
#include "Shader.h"

class SceneNode
{
public:
	SceneNode(Mesh* m = NULL, Vector4 colour = Vector4(1, 1, 1, 1));
	~SceneNode(void);

	void			SetTransform(const Matrix4 &matrix)		{ transform = matrix; }
	const Matrix4	GetTransform() const					{ return transform; }
	Matrix4			GetWorldTransform() const				{ return worldTransform; }

	Vector4			GetColour() const						{ return colour; }
	void			SetColour(Vector4 newColour)			{ colour = newColour; }

	Vector3			GetModelScale() const					{ return modelScale; }
	void			SetModelScale(Vector3 newScale)			{ modelScale = newScale; }

	Mesh*			GetMesh() const							{ return mesh; }
	void			SetMesh(Mesh* newMesh)					{ mesh = newMesh; }

	int				GetIsHeightMap() const					{ return isHeightMap; }
	void			SetIsHeightMap(int value)				{ isHeightMap = value; }

	int				GetIsSkinned() const					{ return isSkinned; }
	void			SetIsSkinned(int value)					{ isSkinned = value; }
	virtual void	SwitchShadowSkinned()					{}
	
	std::vector<SceneNode*>::const_iterator GetChildIteratorStart() { return children.begin(); }

	std::vector<SceneNode*>::const_iterator GetChildIteratorEnd() { return children.end(); }

	void			AddChild(SceneNode* newChild);
	virtual void	Update(float dt);
	virtual void	Draw(const OGLRenderer& r);

	// tutorial 7

	float			GetBoundingRadius() const				{ return boundingRadius; }
	void			SetBoundingRadius(float f)				{ boundingRadius = f; }

	float			GetCameraDistance() const				{ return distanceFromCamera; }
	void			SetCameraDistance(float f)				{ distanceFromCamera = f; }

	void			SetTexture(GLuint tex)					{ texture = tex; }
	GLuint			GetTexture() const						{ return texture; }

	virtual GLuint	GetPlanetTexture();
	virtual GLuint	GetRockTexture();

	void			SetShader(Shader* inputShader)			{ shader = inputShader; }
	Shader*			GetShader()								{ return shader; }

	static bool		CompareByCameraDistance(SceneNode* a, SceneNode* b) {
		return (a->distanceFromCamera < b->distanceFromCamera) ? true : false;
	}

protected:
	SceneNode*				parent;
	Mesh*					mesh;
	Matrix4					worldTransform;
	Matrix4					transform;
	Vector3					modelScale;
	Vector4					colour;
	std::vector<SceneNode*> children;

	// tutorial 7

	float distanceFromCamera;
	float boundingRadius;
	GLuint texture;
	Shader* shader;
	int isHeightMap;
	int isSkinned;
};

