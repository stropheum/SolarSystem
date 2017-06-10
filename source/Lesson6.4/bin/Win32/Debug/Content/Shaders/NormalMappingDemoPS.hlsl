cbuffer CBufferPerFrame
{
	float3 CameraPosition;
	float3 AmbientColor;
	float3 LightDirection;
	float3 LightColor;	
}

cbuffer CBufferPerObject
{
	float3 SpecularColor;
	float SpecularPower;
}

Texture2D ColorTexture;
Texture2D NormalMap;
SamplerState TextureSampler;

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float3 WorldPosition : WORLDPOS;
	float2 TextureCoordinate : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 sampledNormal = (2 * NormalMap.Sample(TextureSampler, IN.TextureCoordinate).xyz) - 1.0; // Map normal from [0..1] to [-1..1]
	float3x3 tbn = float3x3(IN.Tangent, IN.Binormal, IN.Normal);
	sampledNormal = mul(sampledNormal, tbn); // Transform normal to world space

	float3 viewDirection = normalize(CameraPosition - IN.WorldPosition);
	float n_dot_l = dot(sampledNormal, LightDirection);
	float3 halfVector = normalize(LightDirection + viewDirection);
	float n_dot_h = dot(sampledNormal, halfVector);

	float4 color = ColorTexture.Sample(TextureSampler, IN.TextureCoordinate);
	float2 lightCoefficients = lit(n_dot_l, n_dot_h, SpecularPower).yz;

	float3 ambient = color.rgb * AmbientColor;
	float3 diffuse = color.rgb * lightCoefficients.x * LightColor;
	float3 specular = min(lightCoefficients.y, color.w) * SpecularColor;

	return float4(saturate(ambient + diffuse + specular), color.a);
}