#pragma once

#include "Transform.h"
#include "Camera.h"
#include <wrl/client.h>
#include <d3d11.h>
#include "SimpleShader.h"
#include <DirectXMath.h>

#include <memory>

// Helper macro for getting a float between min and max
#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min)

struct Particle
{
	float emitTime;
	DirectX::XMFLOAT3 startPos;
	DirectX::XMFLOAT3 startVel;
	float startRot;
	float startRotVel;
	DirectX::XMFLOAT3 padding;
};

enum EmitterShape {
	Point,
	Sphere,
	Box
};

struct EmitterProperties
{
	float particlesPerSecond;
	float particleLifetime;
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	EmitterShape shape;
	DirectX::XMFLOAT3 shapeDimensions;
	float startSize;
	float endSize;
	DirectX::XMFLOAT3 startVelocity;
	DirectX::XMFLOAT3 acceleration;
	float startRotation;
	float startRotationVelocity;
	float rotationAcceleration;
};

class Emitter
{
public:
	Emitter(
		int maxParticles,
		EmitterProperties props,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler
	);
	~Emitter();

	void Update(float dt, float currentTime);
	void Draw(Camera* camera, float currentTime);

	Transform& GetTransform();

	EmitterProperties& GetProperties();
	void SetProperties(const EmitterProperties& props);
private:
	Transform m_transform;

	int m_maxParticles;
	Particle* m_particles;	// circular buffer of particles

	// info to track circular buffer
	int m_firstLivingParticleIndex;
	int m_firstDeadParticleIndex;
	int m_livingParticleCount;

	// emitter properties
	EmitterProperties m_properties;

	// helper data
	float m_timeAccumulator;

	// DX11 stuff
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
	std::shared_ptr<SimpleVertexShader> m_VS;
	std::shared_ptr<SimplePixelShader> m_PS;

	// helper functions
	void UpdateParticle(float currentTime, int index);
	void EmitParticle(float currentTime);
};