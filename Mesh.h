#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;	// the vertices of the mesh
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;	// the indices of each triangle in the mesh

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	int indexCount;					// how many indices there are

	void MeshHelper(
		Vertex* vertices,				// an array of vertices
		int vertexCount,				// the count of vertices in the array
		unsigned int* indices,			// an array of indices
		int indexCount,					// the count of indices in the array
		Microsoft::WRL::ComPtr<ID3D12Device> device		// a pointer to the device used to make buffers
	);
public:
	Mesh(
		Vertex* vertices,	// an array of vertices
		int vertexCount,	// the count of vertices in the array
		unsigned int* indices,	// an array of indices
		int indexCount,			// the count of indices in the array
		Microsoft::WRL::ComPtr<ID3D12Device> device);	// a pointer to the device used to make buffers
	Mesh(const char* filePath, Microsoft::WRL::ComPtr<ID3D12Device> device);

	~Mesh();

	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer();

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();

	int GetIndexCount();
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

