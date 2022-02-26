#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

class Material
{
public:
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
		DirectX::XMFLOAT3 colorTint,
		DirectX::XMFLOAT2 uvScale,
		DirectX::XMFLOAT2 uvOffset);
	~Material();

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	DirectX::XMFLOAT3 GetColorTint();
	DirectX::XMFLOAT2 GetUvScale();
	DirectX::XMFLOAT2 GetUvOffset();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs();

	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);
	void SetColorTint(DirectX::XMFLOAT3 colorTint);
	void SetUvScale(DirectX::XMFLOAT2 uvScale);
	void SetUvOffset(DirectX::XMFLOAT2 uvOffset);
private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;

	bool finalized;

	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};

