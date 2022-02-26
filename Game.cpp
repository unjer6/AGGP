#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "DX12Helper.h"
#include "BufferStructs.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true),			   // Show extra stats (fps) in title bar?
	vsync(false)
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	camera = Camera((float)this->width / this->height);
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object created in Game
	// We need to wait here until the GPU
	// is actually done with its work
	DX12Helper::GetInstance().WaitForGPU();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	CreateMaterials();
	CreateBasicGeometry();
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature 
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(GetFullPathTo_Wide(L"VertexShader.cso").c_str(),
			vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(GetFullPathTo_Wide(L"PixelShader.cso").c_str(),
			pixelShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[0].SemanticName = "POSITION";            // Name must match semantic in shader
		inputElements[0].SemanticIndex = 0;                    // This is the first POSITION semantic

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;    // R32 G32 = float2
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0;                    // This is the first TEXCOORD semantic

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0;                    // This is the first NORMAL semantic

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0;                    // This is the first TANGENT semantic
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0;  // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		// serialize
		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;
		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((char*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
			D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

void Game::CreateMaterials()
{
	DX12Helper& helper = DX12Helper::GetInstance();

	Material _ = Material(pipelineState, XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Bronze/albedo.png").c_str()), 0);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Bronze/normals.png").c_str()), 1);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Bronze/metal.png").c_str()), 2);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Bronze/roughness.png").c_str()), 3);
	_.FinalizeMaterial();
	materials.push_back(_);

	_ = Material(pipelineState, XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Cobblestone/albedo.png").c_str()), 0);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Cobblestone/normals.png").c_str()), 1);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Cobblestone/metal.png").c_str()), 2);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/Cobblestone/roughness.png").c_str()), 3);
	_.FinalizeMaterial();
	materials.push_back(_);

	_ = Material(pipelineState, XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/ScratchedMetal/albedo.png").c_str()), 0);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/ScratchedMetal/normals.png").c_str()), 1);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/ScratchedMetal/metal.png").c_str()), 2);
	_.AddTexture(helper.LoadTexture(GetFullPathTo_Wide(L"../../Assets/Materials/ScratchedMetal/roughness.png").c_str()), 3);
	_.FinalizeMaterial();
	materials.push_back(_);
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/cube.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/cylinder.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/helix.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/quad.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/quad_double_sided.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/sphere.obj").c_str(), device));
	meshes.push_back(Mesh(GetFullPathTo("../../Assets/Models/torus.obj").c_str(), device));

	entities.push_back(GameEntity(&meshes[5], &materials[0]));	// sphere, bronze
	entities.push_back(GameEntity(&meshes[2], &materials[1]));	// helix, cobblestone
	entities.push_back(GameEntity(&meshes[0], &materials[2]));	// cube, scratched

	entities[0].GetTransform()->SetPosition(-3, 0, 0);
	entities[1].GetTransform()->SetPosition(0, 0, 0);
	entities[2].GetTransform()->SetPosition(3, 0, 0);

	// Lights

	Light _;
	
	_.Type = LIGHT_TYPE_DIRECTIONAL;
	_.Color = XMFLOAT3(1, 1, 1);
	_.Direction = XMFLOAT3(0, -1, 1);
	_.Intensity = 0.5f;
	lights.push_back(_);
	
	_.Type = LIGHT_TYPE_POINT;
	_.Color = XMFLOAT3(1, 0.1f, 0.1f);
	_.Position = XMFLOAT3(-1.5f, 0, 0);
	_.Range = 3;
	_.Intensity = 3;
	lights.push_back(_);

	_.Type = LIGHT_TYPE_POINT;
	_.Color = XMFLOAT3(0.1f, 0.1f, 1);
	_.Position = XMFLOAT3(1.5f, 0, 0);
	_.Range = 3;
	_.Intensity = 3;
	lights.push_back(_);
}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	camera.SetAspectRatio((float)this->width / this->height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	camera.Update(deltaTime);

	for (auto& entity : entities) {
		entity.GetTransform()->Rotate(0, deltaTime * DirectX::XM_PIDIV4 * 0.5f, 0);
	}
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	DX12Helper& dx12Helper = DX12Helper::GetInstance();

	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color
		float color[] = { 0, 0, 0, 1.0f }; //{ 0.4f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, // Max depth = 1.0f
			0, // Not clearing stencil, but need a value
			0, 0); // No scissor rects
	}

	// Rendering here!
	{
		// Set overall pipeline state
		commandList->SetPipelineState(pipelineState.Get());

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = dx12Helper.GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Draw
		VertexBufferExternalData data = {};
		data.view = camera.GetViewMatrix();
		data.projection = camera.GetProjectionMatrix();
		for (auto& entity : entities) {
			data.world = entity.GetTransform()->GetWorldMatrix();
			data.worldInverseTranspose = entity.GetTransform()->GetWorldMatrixInverseTranspose();

			D3D12_GPU_DESCRIPTOR_HANDLE handle = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(&data, sizeof(VertexBufferExternalData));
			commandList->SetGraphicsRootDescriptorTable(0, handle);

			Material* mat = entity.GetMaterial();

			// Pixel shader data and cbuffer setup
			{
				PixelBufferExternalData psData = {};
				psData.uvScale = mat->GetUvScale();
				psData.uvOffset = mat->GetUvOffset();
				psData.cameraPosition = camera.GetTransform()->GetPosition();
				psData.lightCount = lights.size();
				memcpy(psData.lights, &lights[0], sizeof(Light) * psData.lightCount);

				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
				D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psData), sizeof(PixelBufferExternalData));

				// Set this constant buffer handle
				// Note: This assumes that descriptor table 1 is the
				//       place to put this particular descriptor.  This
				//       is based on how we set up our root signature.
				commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
			}

			commandList->SetPipelineState(mat->GetPipelineState().Get());

			// Set the SRV descriptor handle for this material's textures
			// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
			commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForSRVs());			

			commandList->IASetVertexBuffers(0, 1, entity.GetMesh()->GetVertexBufferView());
			commandList->IASetIndexBuffer(entity.GetMesh()->GetIndexBufferView());

			commandList->DrawIndexedInstanced(entity.GetMesh()->GetIndexCount(), 1, 0, 0, 0);
		}
	}

	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Must occur BEFORE present
		dx12Helper.CloseExecuteAndResetCommandList();

		// Present the current back buffer
		swapChain->Present(vsync ? 1 : 0, 0);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}