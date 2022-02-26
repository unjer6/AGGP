#include "ShaderIncludes.hlsli"

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float3 worldPosition : POSITION;
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent  : TANGENT;
};

Texture2D Albedo			:  register(t0);
Texture2D NormalMap			:  register(t1);
Texture2D MetalnessMap		:  register(t2);
Texture2D RoughnessMap		:  register(t3);
SamplerState BasicSampler	:  register(s0);

cbuffer ExternalData : register(b0)
{
	float2 uvScale;
	float2 uvOffset;
	float3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// clean up inputs from VS
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.tangent = normalize(input.tangent - input.normal * dot(input.tangent, input.normal)); // Gram-Schmidt assumes T&N are normalized!

	// get PBR values from maps
	float3 albedo = pow(Albedo.Sample(BasicSampler, input.uv).rgb, 2.2);	//albedo
	float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
	float3 T = input.tangent;
	float3 N = input.normal;
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);
	float3 normal = mul(unpackedNormal, TBN);								//normal
	float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;		//roughness
	float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;		//metalness
	float3 specularColor = lerp(F0_NON_METAL.rrr, albedo.rgb, metalness);	//specular color

	// light calculation
	float3 emittedLight = 0;

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[i];

		float3 toLight = -normalize(light.Direction);
		if (light.Type == LIGHT_TYPE_POINT)
			toLight = normalize(light.Position - input.worldPosition);
		float3 toCam = normalize(cameraPosition - input.worldPosition);
		float intensity = light.Intensity;
		if (light.Type == LIGHT_TYPE_POINT)
			intensity *= Attenuate(light, input.worldPosition);

		// Calculate the light amounts
		float diff = DiffusePBR(normal, toLight);
		float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specularColor);
		// Calculate diffuse with energy conservation
		// (Reflected light doesn't get diffused)
		float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);
		// Combine the final diffuse and specular values for this light
		emittedLight += (balancedDiff * albedo + spec) * intensity * light.Color;
	}

	return pow(float4(emittedLight, 1), 1 / 2.2);		// gamma correct again
}