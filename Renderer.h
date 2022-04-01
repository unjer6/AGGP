#pragma once

#include "Sky.h"
#include "GameEntity.h"
#include "Lights.h"

#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include "SpriteBatch.h"
#include "SpriteFont.h"

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
		const std::vector<Light>& lights,
		std::shared_ptr<Mesh> lightMesh,
		std::shared_ptr<SimpleVertexShader> lightVS,
		std::shared_ptr<SimplePixelShader> lightPS,
		std::shared_ptr<DirectX::SpriteBatch> spriteBatch,
		std::shared_ptr<DirectX::SpriteFont> arial)
		: entities(entities)
		, lights(lights)
	{
		this->device = device;
		this->context = context;
		this->swapChain = swapChain;
		this->backBufferRTV = backBufferRTV;
		this->depthBufferDSV = depthBufferDSV;
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
		this->sky = sky;
		this->lightMesh = lightMesh;
		this->lightVS = lightVS;
		this->lightPS = lightPS;
		this->spriteBatch = spriteBatch;
		this->arial = arial;
	}

	~Renderer() {}

	void PreResize();
	void PostResize(
		unsigned int windowWidth,
		unsigned int windowHeight,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);

	void Render(std::shared_ptr<Camera> camera);
private:
	void DrawPointLights(std::shared_ptr<Camera> camera);
	void DrawUI();

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

	unsigned int windowWidth;
	unsigned int windowHeight;

	std::shared_ptr<Sky> sky;
	const std::vector<std::shared_ptr<GameEntity>>& entities;
	const std::vector<Light>& lights;

	std::shared_ptr<Mesh> lightMesh;
	std::shared_ptr<SimpleVertexShader> lightVS;
	std::shared_ptr<SimplePixelShader> lightPS;

	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	std::shared_ptr<DirectX::SpriteFont> arial;
};

