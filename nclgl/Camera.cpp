#include "Camera.h"
#include "Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt) {
	// get position of mouse
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw   -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch,-90.0f);

	if (yaw < 0)
		yaw += 360.0f;
	if (yaw > 360.0f)
		yaw -= 360.0f;

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	// -1 because in OpenGL forward as -z
	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right   = rotation * Vector3(1, 0, 0);

	float speed = 1000.0f * dt;

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_W))
		position += forward * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_S))
		position -= forward * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_A))
		position -= right * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_D))
		position += right * speed;

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT))
		position.y += speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE))
		position.y -= speed;

	std::cout << "Position: " << position << std::endl;
	std::cout << "yee(360): " << yaw << std::endl;
	std::cout << "pitch(90): " << pitch << std::endl;
}

float Camera::AutoMoveCamera(float dt) {
	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 0, 0));

	// -1 because in OpenGL forward as -z
	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float speed = 100.0f * dt;

	position += forward * speed;

	yaw += 0.01f;
	timePassed += dt;
	return timePassed;
}

Matrix4 Camera::BuildViewMatrix() {
	return 
		Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) *
		Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) *
		Matrix4::Translation(-position);
}