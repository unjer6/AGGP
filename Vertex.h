#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition
//
// You will eventually ADD TO this, and/or make more of these!
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The local position of the vertex
	DirectX::XMFLOAT2 UV;			// the uv coordinate for texturing at this vertex
	DirectX::XMFLOAT3 Normal;		// the normal at this vertex
	DirectX::XMFLOAT3 Tangent;		// the tangent of the surface in positive U direction
};