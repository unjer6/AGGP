cbuffer externalData : register(b0)
{
	matrix view;
	matrix projection;

	float4 startColor;
	float4 endColor;

	float currentTime;
	float lifetime;
	float startSize;
	float endSize;

	float3 acceleration;
	float rotationAcceleration;
};

// Struct representing a single particle
struct Particle
{
	float EmitTime;
	float3 StartPosition;
	float3 StartVelocity;
	float StartRotation;
	float StartRotationVelocity;
	float3 padding;
};


// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

// Defines the output data of our vertex shader
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
	float4 colorTint	: COLOR;
};


// The entry point for our vertex shader
VertexToPixel main(uint id : SV_VertexID)
{
	// Set up output
	VertexToPixel output;

	// Get id info
	uint particleID = id / 4;
	uint cornerID = id % 4;

	// Grab one particle and its starting position
	Particle p = ParticleData.Load(particleID);

	// Calculate the age
	float age = currentTime - p.EmitTime;
	float agePercent = age / lifetime;

	// Perform an incredibly simple particle simulation:
	// Move right a little bit based on the particle's age
	float3 pos = acceleration * age * age / 2.0f + p.StartVelocity * age + p.StartPosition;
	float rotation = rotationAcceleration * age * age / 2.0f + p.StartRotationVelocity * age + p.StartRotation;

	float s, c;
	sincos(rotation, s, c); // One function to calc both sin and cos
	float2x2 rot =
	{
		c, s,
		-s, c
	};

	// Size interpolation
	float size = lerp(startSize, endSize, agePercent);

	// Offsets for the 4 corners of a quad - we'll only
	// use one for each vertex, but which one depends
	// on the cornerID above.
	float2 offsets[4];
	offsets[0] = float2(-1.0f, +1.0f);  // TL
	offsets[1] = float2(+1.0f, +1.0f);  // TR
	offsets[2] = float2(+1.0f, -1.0f);  // BR
	offsets[3] = float2(-1.0f, -1.0f);  // BL

	float2 currentOffset = mul(offsets[cornerID], rot) * size;

	// Billboarding!
	// Offset the position based on the camera's right and up vectors
	pos += float3(view._11, view._12, view._13) * currentOffset.x; // RIGHT
	pos += float3(view._21, view._22, view._23) * currentOffset.y; // UP

	// Calculate output position
	matrix viewProj = mul(projection, view);
	output.position = mul(viewProj, float4(pos, 1.0f));

	// UVs for the 4 corners of a quad - again, only
	// using one for each vertex, but which one depends
	// on the cornerID above.
	float2 uvs[4];
	uvs[0] = float2(0, 0); // TL
	uvs[1] = float2(1, 0); // TR
	uvs[2] = float2(1, 1); // BR
	uvs[3] = float2(0, 1); // BL
	output.uv = uvs[cornerID];

	output.colorTint = lerp(startColor, endColor, agePercent);

	return output;
}