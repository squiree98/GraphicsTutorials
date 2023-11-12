#include "Planet.h"

Planet::Planet(Mesh* sphere, Vector3 newModelScale, float newBoundingRadius, GLuint newTexture, const Matrix4& newTransform, bool orbit) {
	this->mesh = sphere;
	this->colour = Vector4(1,1,1,1);
	parent = NULL;
	modelScale = newModelScale;

	boundingRadius = newBoundingRadius;
	distanceFromCamera = 0.0f;
	texture = newTexture;

	transform = newTransform;

	orbitParent = orbit;
}

void Planet::Update(float dt) {
	transform = transform * Matrix4::Rotation(30.0f * dt, Vector3(0, 1, 0));

	SetTransform(GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(1, 0, 0)));
	if (orbitParent) {
		
	}

	SceneNode::Update(dt);
}