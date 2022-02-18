#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
    isMatricesDirty = false;
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixIdentity());

    position = XMFLOAT3(0, 0, 0);
    pitchYawRoll = XMFLOAT3(0, 0, 0);
    scale = XMFLOAT3(1, 1, 1);
}

Transform::~Transform()
{
}

void Transform::MoveAbsolute(float x, float y, float z)
{
    position.x += x;
    position.y += y;
    position.z += z;

    isMatricesDirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
    XMVECTOR rotatedVector = XMVector3Transform(XMVectorSet(x, y, z, 0), XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z));

    XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), rotatedVector));
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
    pitchYawRoll.x += pitch;
    pitchYawRoll.y += yaw;
    pitchYawRoll.z += roll;

    isMatricesDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
    scale.x *= x;
    scale.y *= y;
    scale.z *= z;

    isMatricesDirty = true;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
    return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
    return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
    return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    if (isMatricesDirty) UpdateMatrices();

    return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrixInverseTranspose()
{
    if (isMatricesDirty) UpdateMatrices();

    return worldMatrixInverseTranspose;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
    XMVECTOR rotatedVector = XMVector3Transform(XMVectorSet(0, 1, 0, 0), XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z));
    XMFLOAT3 output;
    XMStoreFloat3(&output, rotatedVector);
    return output;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
    XMVECTOR rotatedVector = XMVector3Transform(XMVectorSet(1, 0, 0, 0), XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z));
    XMFLOAT3 output;
    XMStoreFloat3(&output, rotatedVector);
    return output;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
    XMVECTOR rotatedVector = XMVector3Transform(XMVectorSet(0, 0, 1, 0), XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z));
    XMFLOAT3 output;
    XMStoreFloat3(&output, rotatedVector);
    return output;
}

void Transform::SetPosition(float x, float y, float z)
{
    position = XMFLOAT3(x, y, z);

    isMatricesDirty = true;
}

void Transform::SetPitchYawRoll(float pitch, float yaw, float roll)
{
    pitchYawRoll = XMFLOAT3(pitch, yaw, roll);

    isMatricesDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
    scale = XMFLOAT3(x, y, z);

    isMatricesDirty = true;
}

void Transform::UpdateMatrices()
{
    XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
    XMMATRIX scaling = XMMatrixScaling(scale.x, scale.y, scale.z);

    XMMATRIX world = scaling * rotation * translation;

    XMStoreFloat4x4(&worldMatrix, world);
    XMStoreFloat4x4(&worldMatrixInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(world)));

    isMatricesDirty = false;
}