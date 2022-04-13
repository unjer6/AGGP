#include "Emitter.h"

// Borrowed from demos
// Credit to Chris Cascioli

Emitter::Emitter(
	int maxParticles,
	EmitterProperties props,
	Microsoft::WRL::ComPtr<ID3D11Device> device, 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	std::shared_ptr<SimpleVertexShader> vs,
	std::shared_ptr<SimplePixelShader> ps,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
  : m_maxParticles(maxParticles),
	m_properties(props),
	m_context(context),
	m_VS(vs),
	m_PS(ps),
	m_texture(texture),
	m_sampler(sampler),
	m_timeAccumulator(0),
	m_livingParticleCount(0),
	m_firstLivingParticleIndex(0),
	m_firstDeadParticleIndex(0)
{
	m_particles = new Particle[m_maxParticles];
	
	// Create an index buffer for particle drawing
	// indices as if we had two triangles per particle
	unsigned int* indices = new unsigned int[m_maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < m_maxParticles * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	// Regular (static) index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned int) * m_maxParticles * 6;
	device->CreateBuffer(&ibDesc, &indexData, m_indexBuffer.GetAddressOf());
	delete[] indices; // Sent to GPU already

	// Make a dynamic buffer to hold all particle data on GPU
	// Note: We'll be overwriting this every frame with new lifetime data
	D3D11_BUFFER_DESC allParticleBufferDesc = {};
	allParticleBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	allParticleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	allParticleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	allParticleBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	allParticleBufferDesc.StructureByteStride = sizeof(Particle);
	allParticleBufferDesc.ByteWidth = sizeof(Particle) * m_maxParticles;
	device->CreateBuffer(&allParticleBufferDesc, 0, m_particleDataBuffer.GetAddressOf());

	// Create an SRV that points to a structured buffer of particles
	// so we can grab this data in a vertex shader
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(m_particleDataBuffer.Get(), &srvDesc, m_particleDataSRV.GetAddressOf());
}

Emitter::~Emitter()
{
	delete[] m_particles;
}

void Emitter::Update(float dt, float currentTime)
{
	if (m_livingParticleCount > 0)
	{
		// Update all particles - Check cyclic buffer first
		if (m_firstLivingParticleIndex < m_firstDeadParticleIndex)
		{
			// First alive is before first dead, so no wrapping
			for (int i = m_firstLivingParticleIndex; i < m_firstDeadParticleIndex; i++)
				UpdateParticle(currentTime, i);
		}
		else if (m_firstDeadParticleIndex < m_firstLivingParticleIndex)
		{
			// Update first half (from firstAlive to max particles)
			for (int i = m_firstLivingParticleIndex; i < m_maxParticles; i++)
				UpdateParticle(currentTime, i);

			// Update second half (from 0 to first dead)
			for (int i = 0; i < m_firstDeadParticleIndex; i++)
				UpdateParticle(currentTime, i);
		}
		else
		{
			// First alive is EQUAL TO first dead, so they're either all alive or all dead
			for (int i = 0; i < m_maxParticles; i++)
				UpdateParticle(currentTime, i);
		}
	}

	m_timeAccumulator += dt;
	float secondsPerParticle = 1.0f / m_properties.particlesPerSecond;
	while (m_timeAccumulator > secondsPerParticle)
	{
		EmitParticle(currentTime);
		m_timeAccumulator -= secondsPerParticle;
	}

	// Copy to GPU
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	m_context->Map(m_particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	// How are living particles arranged in the buffer?
	if (m_firstLivingParticleIndex < m_firstDeadParticleIndex)
	{
		// Only copy from FirstAlive -> FirstDead
		memcpy(
			mapped.pData, // Destination = start of particle buffer
			m_particles + m_firstLivingParticleIndex, // Source = particle array, offset to first living particle
			sizeof(Particle) * m_livingParticleCount); // Amount = number of particles (measured in BYTES!)
	}
	else
	{
		// Copy from 0 -> FirstDead 
		memcpy(
			mapped.pData, // Destination = start of particle buffer
			m_particles, // Source = start of particle array
			sizeof(Particle) * m_firstDeadParticleIndex); // Amount = particles up to first dead (measured in BYTES!)

		// ALSO copy from FirstAlive -> End
		memcpy(
			(void*)((Particle*)mapped.pData + m_firstDeadParticleIndex), // Destination = particle buffer, AFTER the data we copied in previous memcpy()
			m_particles + m_firstLivingParticleIndex,  // Source = particle array, offset to first living particle
			sizeof(Particle) * (m_maxParticles - m_firstLivingParticleIndex)); // Amount = number of living particles at end of array (measured in BYTES!)
	}

	// Unmap now that we're done copying
	m_context->Unmap(m_particleDataBuffer.Get(), 0);
}

void Emitter::Draw(Camera* camera, float currentTime)
{
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = 0;
	m_context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
	m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Shader setup
	m_VS->SetShader();
	m_PS->SetShader();

	// SRVs - Particle data in VS and texture in PS
	m_VS->SetShaderResourceView("ParticleData", m_particleDataSRV);
	m_PS->SetShaderResourceView("Texture", m_texture);

	// Vertex data
	m_VS->SetMatrix4x4("view", camera->GetView());
	m_VS->SetMatrix4x4("projection", camera->GetProjection());
	m_VS->SetFloat("currentTime", currentTime);
	m_VS->SetFloat("lifetime", m_properties.particleLifetime);
	m_VS->SetFloat4("startColor", m_properties.startColor);
	m_VS->SetFloat4("endColor", m_properties.endColor);
	m_VS->SetFloat("startSize", m_properties.startSize);
	m_VS->SetFloat("endSize", m_properties.endSize);
	m_VS->SetFloat3("acceleration", m_properties.acceleration);
	m_VS->SetFloat("rotationAcceleration", m_properties.rotationAcceleration);
	m_VS->CopyAllBufferData();

	// sampler
	m_PS->SetSamplerState("BasicSampler", m_sampler);

	// Now that all of our data is in the beginning of the particle buffer,
	// we can simply draw the correct amount of living particle indices.
	// Each particle = 4 vertices = 6 indices for a quad
	m_context->DrawIndexed(m_livingParticleCount * 6, 0, 0);
}

Transform& Emitter::GetTransform()
{
	return m_transform;
}

EmitterProperties& Emitter::GetProperties()
{
	return m_properties;
}

void Emitter::SetProperties(const EmitterProperties& props)
{
	m_properties = props;
}

void Emitter::UpdateParticle(float currentTime, int index)
{
	float age = currentTime - m_particles[index].emitTime;

	// Update and check for death
	if (age >= m_properties.particleLifetime)
	{
		// Recent death, so retire by moving alive count (and wrap)
		m_firstLivingParticleIndex++;
		m_firstLivingParticleIndex %= m_maxParticles;
		m_livingParticleCount--;
	}
}

void Emitter::EmitParticle(float currentTime)
{
	// Any left to spawn?
	if (m_livingParticleCount == m_maxParticles)
		return;

	// Which particle is spawning?
	int spawnedIndex = m_firstDeadParticleIndex;

	// Update the spawn time of the first dead particle
	m_particles[spawnedIndex].emitTime = currentTime;
	m_particles[spawnedIndex].startVel = m_properties.startVelocity;
	m_particles[spawnedIndex].startRot = m_properties.startRotation;
	m_particles[spawnedIndex].startRotVel = m_properties.startRotationVelocity;

	switch (m_properties.shape) {
		case EmitterShape::Point:
			m_particles[spawnedIndex].startPos = m_transform.GetPosition();
			break;
		case EmitterShape::Box:
		{
			DirectX::XMFLOAT3 offset;
			offset.x = RandomRange(-1, 1) * m_properties.shapeDimensions.x * 0.5f;
			offset.y = RandomRange(-1, 1) * m_properties.shapeDimensions.y * 0.5f;
			offset.z = RandomRange(-1, 1) * m_properties.shapeDimensions.z * 0.5f;
			DirectX::XMFLOAT3 pos = m_transform.GetPosition();
			pos.x += offset.x;
			pos.y += offset.y;
			pos.z += offset.z;
			m_particles[spawnedIndex].startPos = pos;
			break;
		}
		case EmitterShape::Sphere:
		{
			DirectX::XMFLOAT3 offset(1,1,1);
			// this may look dumb but its actually a pretty quick way to find a uniform point in a sphere
			while (offset.x * offset.x + offset.y * offset.y + offset.z * offset.z > 1) {
				offset.x = RandomRange(-1, 1);
				offset.y = RandomRange(-1, 1);
				offset.z = RandomRange(-1, 1);
			}
			offset.x *= m_properties.shapeDimensions.x * 0.5f;
			offset.y *= m_properties.shapeDimensions.y * 0.5f;
			offset.z *= m_properties.shapeDimensions.z * 0.5f;
			DirectX::XMFLOAT3 pos = m_transform.GetPosition();
			pos.x += offset.x;
			pos.y += offset.y;
			pos.z += offset.z;
			m_particles[spawnedIndex].startPos = pos;
			break;
		}
	}

	// Increment the first dead particle (since it's now alive)
	m_firstDeadParticleIndex++;
	m_firstDeadParticleIndex %= m_maxParticles; // Wrap

	// One more living particle
	m_livingParticleCount++;
}
