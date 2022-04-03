#include "Lighting.hlsli"

// How many lights could we handle?
#define MAX_LIGHTS 128

// Data that only changes once per frame
cbuffer perFrame : register(b0)
{
	// An array of light data
	Light lights[MAX_LIGHTS];

	// The amount of lights THIS FRAME
	int lightCount;

	// Needed for specular (reflection) calculation
	float3 cameraPosition;

	// The number of mip levels in the specular IBL map
	int SpecIBLTotalMipLevels;

	float2 screenSize;
};

// Data that can change per material
cbuffer perMaterial : register(b1)
{
	// Surface color
	float4 colorTint;
};


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION; // The world position of this PIXEL
};


// Texture-related variables
Texture2D NormalMap			: register(t0);
Texture2D RoughnessMap		: register(t1);

// Environment map for reflections
TextureCube SpecularIBLMap		: register(t2);

// Refraction requirement
Texture2D ScreenPixels			: register(t3);

// Samplers
SamplerState BasicSampler		: register(s0);
SamplerState ClampSampler		: register(s1);


// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	// Always re-normalize interpolated direction vectors
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);

	// The actual screen UV and refraction offset UV
	float2 screenUV = input.screenPosition.xy / screenSize;
	float2 offsetUV = NormalMap.Sample(BasicSampler, input.uv).xy * 2 - 1;

	// Flip the offset's V upside down due to world <--> texture space difference
	offsetUV.y *= -1;

	// Final refracted UV (where to pull the pixel color)
	// If not using refraction at all, just use the screen UV itself
	float2 refractedUV = screenUV + offsetUV * 0.1f /*refraction scale*/;

	// Get the color at the (now verified) offset UV
	float3 sceneColor = pow(ScreenPixels.Sample(ClampSampler, refractedUV).rgb, 2.2f); // Un-gamma correct

	// Get reflections
	float3 viewToCam = normalize(cameraPosition - input.worldPos);
	float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
	float3 envSample = SpecularIBLMap.Sample(BasicSampler, viewRefl).rgb;

	// Determine the reflectivity based on viewing angle
	// using the Schlick approximation of the Fresnel term
	float fresnel = Fresnel(input.normal, viewToCam, F0_NON_METAL);
	return float4(pow(lerp(sceneColor, envSample, fresnel), 1.0f / 2.2f), 1); // Re-gamma correct after linear interpolation

}