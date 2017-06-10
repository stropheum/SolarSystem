cbuffer CBufferPerFrame
{
	float3 CameraPosition;
	float3 AmbientColor;
	float3 LightDirection;
	float3 LightColor;
};

cbuffer CBufferPerObject
{
	float3 SpecularColor;
	float SpecularPower;
}

Texture2D ColorAndSpecularMap;
SamplerState TextureSampler;

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TextureCoordinates : TEXCOORD;
	float3 Normal : NORMAL;
	float3 WorldPosition : WORLDPOS;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float3 normal = normalize(IN.Normal);
	float3 viewDirection = normalize(CameraPosition - IN.WorldPosition);
	float n_dot_l = dot(LightDirection, normal);
	float3 halfVector = normalize(LightDirection + viewDirection);
	float n_dot_h = dot(normal, halfVector);

	float4 color = ColorAndSpecularMap.Sample(TextureSampler, IN.TextureCoordinates);
	float2 lightCoefficients = lit(n_dot_l, n_dot_h, SpecularPower).yz;

	float3 ambient = color.rgb * AmbientColor;
	float3 diffuse = color.rgb * lightCoefficients.x * LightColor;
	float3 specular = min(lightCoefficients.y, color.w) * SpecularColor;

	return float4(saturate(ambient + diffuse + specular), color.a);
}