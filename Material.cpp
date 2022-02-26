#include "Material.h"
#include "DX12Helper.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT3 colorTint, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset)
{
	this->pipelineState = pipelineState;
	this->colorTint = colorTint;
	this->uvScale = uvScale;
	this->uvOffset = uvOffset;

	finalized = false;
}

Material::~Material()
{
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	finalized = false;
	textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
	if (finalized)
		return;

	DX12Helper& helper = DX12Helper::GetInstance();
	for (int i = 0; i < 4; i++) {
		D3D12_GPU_DESCRIPTOR_HANDLE handle = helper.CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);

		if (i == 0)
			finalGPUHandleForSRVs = handle;
	}

	finalized = true;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
	return pipelineState;
}

DirectX::XMFLOAT3 Material::GetColorTint()
{
	return colorTint;
}

DirectX::XMFLOAT2 Material::GetUvScale()
{
	return uvScale;
}

DirectX::XMFLOAT2 Material::GetUvOffset()
{
	return uvOffset;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs()
{
	return finalGPUHandleForSRVs;
}

void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	this->pipelineState = pipelineState;
}

void Material::SetColorTint(DirectX::XMFLOAT3 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetUvScale(DirectX::XMFLOAT2 uvScale)
{
	this->uvScale = uvScale;
}

void Material::SetUvOffset(DirectX::XMFLOAT2 uvOffset)
{
	this->uvOffset = uvOffset;
}
