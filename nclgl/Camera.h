#pragma once
#include "Matrix4.h"
#include "Vector3.h"

class Camera
{
public:
	Camera(void) {
		yaw = 0.0f;
		pitch = 0.0f;
	};

	Camera(float pitch, float yaw, Vector3 position) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;
		this->timePassed = 0;
		this->sceneNumber = 1;
	}

	~Camera(void) {};

	void UpdateCamera(float dt = 1.0f);

	void AutoUpdateCamera(float dt = 1.0f);

	// methods for auto move camera
	void ViewSkinnedMesh(float dt);

	Matrix4 BuildViewMatrix();

	Vector3 GetPosition() const { return position; }
	void SetPosition(Vector3 val) { position = val; }

	float GetYaw() const { return yaw; }
	void SetYaw(float y) { yaw = y; }

	float GetPitch() const { return pitch; }
	void SetPitch(float p) { pitch = p; }

	void ResetScene() { this->sceneNumber = 1; }

protected:
	float yaw;
	float pitch;
	float timePassed;

	int sceneNumber;

	Vector3 position;
};