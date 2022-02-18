#pragma once

#include <DirectXMath.h>
#include "Transform.h"

class Camera
{
public:
	Camera(
		float aspectRatio = 1.0f,
		DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f),
		float fov = DirectX::XM_PIDIV2,
		float nearPlane = 0.001f,
		float farPlane = 100.0f,
		float movementSpeed = 5.0f,
		float lookSpeed = 5.0f
	);
	~Camera();

	void Update(float dt);

	//getters

	Transform* GetTransform();
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();

	float GetAspectRatio();
	float GetFOV();
	float GetNearPlane();
	float GetFarPlane();
	float GetMovementSpeed();
	float GetLookSpeed();

	//setters

	void SetAspectRatio(float aspectRatio);
	void SetFOV(float fov);
	void SetNearPlane(float nearPlane);
	void SetFarPlane(float farPlane);
	void SetMovementSpeed(float movementSpeed);
	void SetLookSpeed(float lookSpeed);

private:
	Transform transfrom;
	DirectX::XMFLOAT4X4 viewMatrix;
	bool isProjectionMatrixDirty;
	DirectX::XMFLOAT4X4 projectionMatrix;

	float aspectRatio;	// width / height
	float fov;			// radians
	float nearPlane;
	float farPlane;
	float movementSpeed;
	float lookSpeed;

	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
};