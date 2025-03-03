#pragma once

#include "Sky.h"
#include "GameEntity.h"
#include "Emitter.h"
#include "Lights.h"

#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include "SpriteBatch.h"
#include "SpriteFont.h"

//
// Code borrowed from Github Demo Repo
// https://github.com/vixorien/ggp-advanced-demos/blob/main/Refraction/Renderer.h
//

class Renderer
{
public:
	Renderer(
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int windowWidth,
		unsigned int windowHeight,
		std::shared_ptr<Sky> sky,
		const std::vector<std::shared_ptr<GameEntity>>& entities,
		const std::vector<std::shared_ptr<Emitter>>& emitters,
		const std::vector<Light>& lights,
		std::shared_ptr<Mesh> lightMesh,
		std::shared_ptr<SimpleVertexShader> lightVS,
		std::shared_ptr<SimplePixelShader> lightPS,
		std::shared_ptr<DirectX::SpriteBatch> spriteBatch,
		std::shared_ptr<DirectX::SpriteFont> arial,
		std::shared_ptr<SimpleVertexShader> fullScreenVS,
		std::shared_ptr<SimplePixelShader> texturePS,
		std::shared_ptr<SimpleVertexShader> shadowVS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler);

	~Renderer();

	void PreResize();
	void PostResize(
		unsigned int windowWidth,
		unsigned int windowHeight,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);

	void Render(std::shared_ptr<Camera> camera, float totalTime);

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetSceneColorRTV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneColorSRV();
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetSceneNormalsRTV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneNormalsSRV();
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetSceneDepthRTV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSceneDepthSRV();

	void ResizeShadowMap(unsigned int shadowMapSize);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShadowMapSRV();
	unsigned int GetShadowMapResolution();
	float GetShadowProjectionSize();

	void SetShadowMapResolution(unsigned int resolution);
	void SetShadowProjectionSize(float projectionSize);
private:
	void DrawPointLights(std::shared_ptr<Camera> camera);
	void DrawUI();

	void CreateRenderTarget(
		unsigned int width,
		unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

	unsigned int windowWidth;
	unsigned int windowHeight;

	std::shared_ptr<Sky> sky;
	const std::vector<std::shared_ptr<GameEntity>>& entities;
	const std::vector<std::shared_ptr<Emitter>>& emitters;
	const std::vector<Light>& lights;

	std::shared_ptr<Mesh> lightMesh;
	std::shared_ptr<SimpleVertexShader> lightVS;
	std::shared_ptr<SimplePixelShader> lightPS;

	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	std::shared_ptr<DirectX::SpriteFont> arial;

	// Render Targets for refraction (this could also be a vector or map)
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneColorSRV;

	// Shadow resources
	int shadowMapResolution;
	float shadowProjectionSize;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;
	void CreateShadowMapResources(unsigned int shadowMapSize, float projectionSize);
	void UpdateShadowProjection(float projectionSize);
	void UpdateShadowView(const Light* light);
	void RenderShadowMap();

	// Extra render targets just for the fun of it (displayed in ImGui)
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalsRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalsSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthSRV;

	// material information for a fullscreen texture copy
	std::shared_ptr<SimpleVertexShader> fullScreenVS;
	std::shared_ptr<SimplePixelShader> texturePS;
	std::shared_ptr<SimpleVertexShader> shadowVS;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;

	// Particle states
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;
};

