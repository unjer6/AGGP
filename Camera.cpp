#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(
	float aspectRatio,
	XMFLOAT3 position,
	float fov,
	float nearPlane,
	float farPlane,
	float movementSpeed,
	float lookSpeed
) :
	aspectRatio(aspectRatio),
	fov(fov),
	nearPlane(nearPlane),
	farPlane(farPlane),
	movementSpeed(movementSpeed),
	lookSpeed(lookSpeed),
	isProjectionMatrixDirty(true)
{
	transfrom.SetPosition(position.x, position.y, position.z);
	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

Camera::~Camera()
{
}

void Camera::Update(float dt)
{
	Input& input = Input::GetInstance();

	float finalMovementSpeed = movementSpeed * dt;
	if (input.KeyDown(VK_SHIFT))
		finalMovementSpeed *= 2;

	float finalLookSpeed = lookSpeed * dt;

	if (input.KeyDown('W')) transfrom.MoveRelative(0.0f, 0.0f, finalMovementSpeed);
	if (input.KeyDown('S')) transfrom.MoveRelative(0.0f, 0.0f, -finalMovementSpeed);
	if (input.KeyDown('A')) transfrom.MoveRelative(-finalMovementSpeed, 0.0f, 0.0f);
	if (input.KeyDown('D')) transfrom.MoveRelative(finalMovementSpeed, 0.0f, 0.0f);
	if (input.KeyDown(' ')) transfrom.MoveAbsolute(0.0f, finalMovementSpeed, 0.0f);
	if (input.KeyDown(VK_CONTROL)) transfrom.MoveAbsolute(0.0f, -finalMovementSpeed, 0.0f);

	if (input.MouseLeftDown())
	{
		float yRotation = input.GetMouseXDelta() * lookSpeed * 0.001f;
		float xRotation = input.GetMouseYDelta() * lookSpeed * 0.001f;
		transfrom.Rotate(xRotation, yRotation, 0);

		XMFLOAT3 rotation = transfrom.GetPitchYawRoll();
		if (rotation.x > XM_PIDIV2) transfrom.SetPitchYawRoll(XM_PIDIV2, rotation.y, rotation.z);
		if (rotation.x < -XM_PIDIV2) transfrom.SetPitchYawRoll(-XM_PIDIV2, rotation.y, rotation.z);
	}
}

Transform* Camera::GetTransform()
{
	return &transfrom;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	UpdateViewMatrix();		// we always assume our transform data is dirty and we need a new view matrix

	return viewMatrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	if (isProjectionMatrixDirty)
		UpdateProjectionMatrix();

	return projectionMatrix;
}

float Camera::GetAspectRatio()
{
	return aspectRatio;
}

float Camera::GetFOV()
{
	return fov;
}

float Camera::GetNearPlane()
{
	return nearPlane;
}

float Camera::GetFarPlane()
{
	return farPlane;
}

float Camera::GetMovementSpeed()
{
	return movementSpeed;
}

float Camera::GetLookSpeed()
{
	return lookSpeed;
}

void Camera::SetAspectRatio(float aspectRatio)
{
	this->aspectRatio = aspectRatio;

	isProjectionMatrixDirty = true;
}

void Camera::SetFOV(float fov)
{
	this->fov = fov;

	isProjectionMatrixDirty = true;
}

void Camera::SetNearPlane(float nearPlane)
{
	this->nearPlane = nearPlane;

	isProjectionMatrixDirty = true;
}

void Camera::SetFarPlane(float farPlane)
{
	this->farPlane = farPlane;

	isProjectionMatrixDirty = true;
}

void Camera::SetMovementSpeed(float movementSpeed)
{
	this->movementSpeed = movementSpeed;
}

void Camera::SetLookSpeed(float lookSpeed)
{
	this->lookSpeed = lookSpeed;
}

void Camera::UpdateViewMatrix()
{
	XMFLOAT3 position = transfrom.GetPosition();
	XMVECTOR eyePosition = XMLoadFloat3(&position);

	XMFLOAT3 forward = transfrom.GetForward();
	XMVECTOR eyeDirection = XMLoadFloat3(&forward);

	XMFLOAT3 up = transfrom.GetUp();
	XMVECTOR upDirection = XMLoadFloat3(&up);

	XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(eyePosition, eyeDirection, upDirection));
}

void Camera::UpdateProjectionMatrix()
{
	XMStoreFloat4x4(&projectionMatrix, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));

	isProjectionMatrixDirty = false;
}
