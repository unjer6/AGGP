#include "Renderer.h"

#include <DirectXMath.h>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx11.h"
#include "ImGUI/imgui_impl_win32.h"

//
// Code borrowed from Github Demo Repo
// https://github.com/vixorien/ggp-advanced-demos/blob/main/Refraction/Renderer.cpp
//

Renderer::Renderer(
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
	Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler)
  : entities(entities),
	emitters(emitters),
	lights(lights)
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
	this->fullScreenVS = fullScreenVS;
	this->texturePS = texturePS;
	this->shadowVS = shadowVS;
	this->basicSampler = basicSampler;

	PostResize(windowWidth, windowHeight, backBufferRTV, depthBufferDSV);

	// Set up render states for particles (since all emitters might use similar ones)
	D3D11_DEPTH_STENCIL_DESC particleDepthDesc = {};
	particleDepthDesc.DepthEnable = true; // READ from depth buffer
	particleDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth WRITING
	particleDepthDesc.DepthFunc = D3D11_COMPARISON_LESS; // Standard depth comparison
	device->CreateDepthStencilState(&particleDepthDesc, particleDepthState.GetAddressOf());

	// Additive blend state for particles (Not every emitter is necessarily additively blended!)
	D3D11_BLEND_DESC additiveBlendDesc = {};
	additiveBlendDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; // Add both colors
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD; // Add both alpha values
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;   // 100% of source color
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;  // 100% of destination color
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;   // 100% of source alpha
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;  // 100% of destination alpha
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&additiveBlendDesc, particleBlendAdditive.GetAddressOf());
	
	// Create shadow map resources
	CreateShadowMapResources(1024, 10.0f);
}

Renderer::~Renderer()
{
}

void Renderer::PreResize()
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
}

void Renderer::PostResize(
	unsigned int windowWidth,
	unsigned int windowHeight,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
{
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;

	sceneColorRTV.Reset();
	sceneColorSRV.Reset();
	sceneNormalsRTV.Reset();
	sceneNormalsSRV.Reset();
	sceneDepthRTV.Reset();
	sceneDepthSRV.Reset();

	CreateRenderTarget(windowWidth, windowHeight, sceneColorRTV, sceneColorSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneNormalsRTV, sceneNormalsSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneDepthRTV, sceneDepthSRV, DXGI_FORMAT_R32_FLOAT);
}

void Renderer::Render(std::shared_ptr<Camera> camera, float totalTime)
{
	// Background color for clearing
	const float color[4] = { 0, 0, 0, 1 };

	// Clear all render targets and the depth stencil
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearRenderTargetView(sceneColorRTV.Get(), color);
	context->ClearRenderTargetView(sceneNormalsRTV.Get(), color);
	context->ClearRenderTargetView(sceneDepthRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	// Render our shadow map
	RenderShadowMap();

	// declare stores for useful data
	const int numTargets = 4;
	ID3D11RenderTargetView* targets[numTargets] = {};
	std::vector<std::shared_ptr<GameEntity>> refractiveEntites;

	// Draw all normal entities
	targets[0] = sceneColorRTV.Get();
	targets[1] = sceneNormalsRTV.Get();
	targets[2] = sceneDepthRTV.Get();
	context->OMSetRenderTargets(3, targets, depthBufferDSV.Get());
	for (auto& ge : entities)
	{
		if (ge->GetMaterial()->GetRefractive()) {
			refractiveEntites.push_back(ge);
			continue;
		}

		std::shared_ptr<SimpleVertexShader> vs = ge->GetMaterial()->GetVertexShader();
		vs->SetMatrix4x4("shadowView", shadowViewMatrix);
		vs->SetMatrix4x4("shadowProjection", shadowProjectionMatrix);

		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lights.size());
		ps->SetInt("lightCount", lights.size());
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("SpecIBLTotalMipLevels", sky->GetIBLMipLevels());
		ps->CopyBufferData("perFrame");

		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetLookUpTable());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetSpecularMap());

		ps->SetShaderResourceView("ShadowMap", shadowSRV);
		ps->SetSamplerState("ShadowSampler", shadowSampler);

		// Draw the entity
		ge->Draw(context, camera);
	}
	
	// Draw the sky
	sky->Draw(camera);

	// Draw scene color to back buffer
	{
		targets[0] = backBufferRTV.Get();
		context->OMSetRenderTargets(1, targets, 0);
		fullScreenVS->SetShader();
		texturePS->SetShader();
		texturePS->SetShaderResourceView("Pixels", sceneColorSRV.Get());
		context->Draw(3, 0);
	}

	// Draw all refractive entites
	targets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, targets, depthBufferDSV.Get());
	for (auto& ge : refractiveEntites) {
		ge->GetMaterial()->SetColorTint(DirectX::XMFLOAT3(1, 0.3, 0.3));

		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lights.size());
		ps->SetInt("lightCount", lights.size());
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("SpecIBLTotalMipLevels", sky->GetIBLMipLevels());
		ps->SetFloat2("screenSize", DirectX::XMFLOAT2(windowWidth, windowHeight));
		ps->CopyBufferData("perFrame");
		
		//ps->SetShaderResourceView("BrdfLookUpMap", sky->GetLookUpTable());
		//ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetSpecularMap());
		ps->SetShaderResourceView("ScreenPixels", sceneColorSRV);

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the light sources
	DrawPointLights(camera);

	// Draw particles =====
	{
		// Ensure we have the back buffer AND the depth buffer bound
		targets[0] = backBufferRTV.Get();
		context->OMSetRenderTargets(1, targets, depthBufferDSV.Get());

		// Set up render states
		context->OMSetBlendState(particleBlendAdditive.Get(), 0, 0xFFFFFFFF);
		context->OMSetDepthStencilState(particleDepthState.Get(), 0);

		// Loop and draw each emitter
		for (auto& e : emitters)
		{
			e->Draw(camera.get(), totalTime);
		}

		// Reset render states
		context->OMSetBlendState(0, 0, 0xFFFFFFFF);
		context->OMSetDepthStencilState(0, 0);
	}

	// Draw some UI
	DrawUI();

	// Draw ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	// Unbind all SRVs at the end of the frame so they're not still bound for input
	// when we begin the MRTs of the next frame
	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> Renderer::GetSceneColorRTV()
{
	return sceneColorRTV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneColorSRV()
{
	return sceneColorSRV;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> Renderer::GetSceneNormalsRTV()
{
	return sceneNormalsRTV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneNormalsSRV()
{
	return sceneNormalsSRV;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> Renderer::GetSceneDepthRTV()
{
	return sceneDepthRTV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSceneDepthSRV()
{
	return sceneDepthSRV;
}

void Renderer::DrawPointLights(std::shared_ptr<Camera> camera)
{
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightVS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lights.size(); i++)
	{
		Light light = lights[i];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		float scale = light.Range / 20.0f;

		// Make the transform for this light
		DirectX::XMMATRIX rotMat = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(scale, scale, scale);
		DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		DirectX::XMMATRIX worldMat = scaleMat * rotMat * transMat;

		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 worldInvTrans;
		DirectX::XMStoreFloat4x4(&world, worldMat);
		DirectX::XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		DirectX::XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}
}

void Renderer::DrawUI()
{
	spriteBatch->Begin();

	// Basic controls
	float h = 10.0f;
	arial->DrawString(spriteBatch.get(), L"Controls:", DirectX::XMVectorSet(10, h, 0, 0));
	arial->DrawString(spriteBatch.get(), L" (WASD, X, Space) Move camera", DirectX::XMVectorSet(10, h + 20, 0, 0));
	arial->DrawString(spriteBatch.get(), L" (Left Click & Drag) Rotate camera", DirectX::XMVectorSet(10, h + 40, 0, 0));
	arial->DrawString(spriteBatch.get(), L" (Left Shift) Hold to speed up camera", DirectX::XMVectorSet(10, h + 60, 0, 0));
	arial->DrawString(spriteBatch.get(), L" (Left Ctrl) Hold to slow down camera", DirectX::XMVectorSet(10, h + 80, 0, 0));
	arial->DrawString(spriteBatch.get(), L" (TAB) Randomize lights", DirectX::XMVectorSet(10, h + 100, 0, 0));

	// Current "scene" info
	h = 150;
	arial->DrawString(spriteBatch.get(), L"Scene Details:", DirectX::XMVectorSet(10, h, 0, 0));
	arial->DrawString(spriteBatch.get(), L" Top: PBR materials", DirectX::XMVectorSet(10, h + 20, 0, 0));
	arial->DrawString(spriteBatch.get(), L" Bottom: Non-PBR materials", DirectX::XMVectorSet(10, h + 40, 0, 0));

	spriteBatch->End();

	// Reset render states, since sprite batch changes these!
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);
}

// Credit to Chirs Cascioli, https://github.com/vixorien/ggp-advanced-demos/blob/main/Refraction/Renderer.cpp
void Renderer::CreateRenderTarget(
	unsigned int width, 
	unsigned int height, 
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, 
	DXGI_FORMAT colorFormat)
{
	// Make the texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // Need both!
	texDesc.Format = colorFormat;
	texDesc.MipLevels = 1; // Usually no mip chain needed for render targets
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1; // Can't be zero
	device->CreateTexture2D(&texDesc, 0, rtTexture.GetAddressOf());

	// Make the render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // This points to a Texture2D
	rtvDesc.Texture2D.MipSlice = 0;                             // Which mip are we rendering into?
	rtvDesc.Format = texDesc.Format;                // Same format as texture
	device->CreateRenderTargetView(rtTexture.Get(), &rtvDesc, rtv.GetAddressOf());

	// Create the shader resource view using default options 
	device->CreateShaderResourceView(
		rtTexture.Get(),     // Texture resource itself
		0,                   // Null description = default SRV options
		srv.GetAddressOf()); // ComPtr<ID3D11ShaderResourceView>
}

void Renderer::CreateShadowMapResources(unsigned int shadowMapSize, float projectionSize)
{
	// Create the initial shadow map
	ResizeShadowMap(shadowMapSize);

	// Create the special "comparison" sampler state for shadows
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // COMPARISON filter!
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// Create a rasterizer state
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Multiplied by (smallest possible positive value storable in the depth buffer)
	shadowRastDesc.DepthBiasClamp = 0.0f;
	shadowRastDesc.SlopeScaledDepthBias = 1.0f;
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	// Create the "camera" matrices for the shadow map rendering

	// View matrix will be re-created each frame in the event the light rotates

	// Projection
	UpdateShadowProjection(projectionSize);
}


void Renderer::UpdateShadowProjection(float projectionSize)
{
	shadowProjectionSize = projectionSize;

	DirectX::XMMATRIX shProj = DirectX::XMMatrixOrthographicLH(shadowProjectionSize, shadowProjectionSize, 0.1f, 100.0f);
	XMStoreFloat4x4(&shadowProjectionMatrix, shProj);
}


void Renderer::UpdateShadowView(const Light* light)
{
	DirectX::XMFLOAT3 lightPos(light->Direction.x * -20, light->Direction.y * -20, light->Direction.z * -20);

	DirectX::XMMATRIX shView = DirectX::XMMatrixLookToLH(
		DirectX::XMLoadFloat3(&lightPos),
		DirectX::XMLoadFloat3(&light->Direction),
		DirectX::XMVectorSet(0, 1, 0, 0));
	DirectX::XMStoreFloat4x4(&shadowViewMatrix, shView);
}



void Renderer::ResizeShadowMap(unsigned int shadowMapSize)
{
	// Reset com ptrs
	shadowSRV.Reset();
	shadowDSV.Reset();

	// Save resolution
	shadowMapResolution = shadowMapSize;

	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution;
	shadowDesc.Height = shadowMapResolution;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture.Get(), &shadowDSDesc, shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture.Get(), &srvDesc, shadowSRV.GetAddressOf());
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetShadowMapSRV() { return shadowSRV; }
unsigned int Renderer::GetShadowMapResolution() { return shadowMapResolution; }
float Renderer::GetShadowProjectionSize() { return shadowProjectionSize; }

void Renderer::SetShadowMapResolution(unsigned int resolution) { ResizeShadowMap(resolution); }
void Renderer::SetShadowProjectionSize(float projectionSize) { UpdateShadowProjection(projectionSize); }


void Renderer::RenderShadowMap()
{
	// Update the shadow view matrix to match the first directional light
	UpdateShadowView(&lights[0]);

	// Initial pipeline setup - No RTV necessary - Clear shadow map
	context->OMSetRenderTargets(0, 0, shadowDSV.Get());
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->RSSetState(shadowRasterizer.Get());

	// Need to create a viewport that matches the shadow map resolution
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	// Turn on our shadow map Vertex Shader
	// and turn OFF the pixel shader entirely
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", shadowViewMatrix);
	shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
	shadowVS->CopyBufferData("perFrame");
	context->PSSetShader(0, 0, 0); // No PS

	// Loop and draw all entities
	for (auto& e : entities)
	{
		shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		shadowVS->CopyBufferData("perObject");

		// Draw the mesh
		e->GetMesh()->SetBuffersAndDraw(context);
	}

	// After rendering the shadow map, go back to the screen
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->RSSetState(0);
}