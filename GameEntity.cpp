#include "GameEntity.h"

GameEntity::GameEntity(Mesh* mesh)
{
	this->mesh = mesh;
}

GameEntity::~GameEntity()
{
}

Transform* GameEntity::GetTransform()
{
	return &transform;
}

Mesh* GameEntity::GetMesh()
{
	return mesh;
}

void GameEntity::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
}
