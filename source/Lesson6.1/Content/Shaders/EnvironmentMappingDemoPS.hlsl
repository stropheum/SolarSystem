cbuffer CBufferPerFrame
{
	float3 AmbientColor;
	float3 EnvColor;
}

cbuffer CBufferPerObject
{
	float ReflectionAmount;
}

Texture2D ColorTexture;
TextureCube EnvironmentMap;
SamplerState TextureSampler;

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float2 TextureCoordinate : TEXCOORD;
	float3 ReflectionVector : REFLECTION;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float4 color = ColorTexture.Sample(TextureSampler, IN.TextureCoordinate);
	float3 ambient = color.rgb * AmbientColor;
	float3 environment = EnvironmentMap.Sample(TextureSampler, IN.ReflectionVector).rgb * EnvColor;

	return float4(lerp(ambient, environment, ReflectionAmount), color.a);
}