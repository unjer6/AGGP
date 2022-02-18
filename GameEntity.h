#pragma once

#include "Mesh.h"
#include "Transform.h"

class GameEntity
{
public:
	GameEntity(Mesh* mesh);
	~GameEntity();

	Transform* GetTransform();
	Mesh* GetMesh();

	void SetMesh(Mesh* mesh);

private:

	Transform transform;
	Mesh* mesh;
};

