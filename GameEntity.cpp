#include "GameEntity.h"

GameEntity::GameEntity(Mesh* mesh, Material* material)
{
	this->mesh = mesh;
	this->material = material;
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

Material* GameEntity::GetMaterial()
{
	return material;
}

void GameEntity::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
}

void GameEntity::SetMaterial(Material* material)
{
	this->material = material;
}
