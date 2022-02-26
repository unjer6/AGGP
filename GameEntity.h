#pragma once

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class GameEntity
{
public:
	GameEntity(Mesh* mesh, Material* material);
	~GameEntity();

	Transform* GetTransform();
	Mesh* GetMesh();
	Material* GetMaterial();

	void SetMesh(Mesh* mesh);
	void SetMaterial(Material* material);

private:

	Transform transform;
	Mesh* mesh;
	Material* material;
};

