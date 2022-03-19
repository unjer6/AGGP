#pragma once

#include <memory>

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <wrl/client.h> // Used for ComPtr

class Sky
{
public:

	// Constructor that loads a DDS cube map file
	Sky(
		const wchar_t* cubemapDDSFile, 
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 	
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	// Constructor that loads 6 textures and makes a cube map
	Sky(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> IBLIrradianceMapPS,
		std::shared_ptr<SimplePixelShader> IBLSpecularConvolutionPS,
		std::shared_ptr<SimplePixelShader> IBLBrdfLookUpTablePS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	~Sky();

	void Draw(std::shared_ptr<Camera> camera);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSpecularMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetLookUpTable();
	int GetIBLMipLevels();

private:

	void InitRenderStates();

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	// Skybox related resources
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimplePixelShader> skyPS;
	
	std::shared_ptr<Mesh> skyMesh;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceMapIBL;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularMapIBL;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdfLookUpTableIBL;
	int specularMipLevels;
	const int numSkippedMipLevels = 3;
	const int cubeFaceSize = 512;
	const int lookUpSize = 512;

	void IBLCreateIrradianceMap(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> IBLIrradianceMapPS);
	void IBLCreateConvolvedSpecularMap(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> IBLSpecularConvolutionPS);
	void IBLCreateBRDFLookUpTexture(
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> IBLBrdfLookUpTablePS);
};

