cbuffer CBufferPerFrame
{
	float3 CameraPosition;
	float3 AmbientColor;
	float3 LightDirection;
	float3 LightColor;
	float3 FogColor;	
}

cbuffer CBufferPerObject
{
	float3 SpecularColor;
	float SpecularPower;
}

Texture2D ColorTexture;
Texture2D SpecularMap;
Texture2D TransparencyMap;
SamplerState TextureSampler;

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float3 WorldPosition : WORLDPOS;
	float2 TextureCoordinate : TEXCOORD;
	float3 Normal : NORMAL;
	float FogAmount : FOG;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	if (IN.FogAmount == 1.0f)
	{
		return float4(FogColor.rgb, 1.0f);
	}

	float sampledAlpha = TransparencyMap.Sample(TextureSampler, IN.TextureCoordinate).a;
	if (sampledAlpha == 0.0f)
	{
		return float4(1.0f, 1.0f, 1.0f, 0.0f);
	}

	float3 normal = normalize(IN.Normal);
	float3 viewDirection = normalize(CameraPosition - IN.WorldPosition);
	float n_dot_l = dot(normal, LightDirection);
	float3 halfVector = normalize(LightDirection + viewDirection);
	float n_dot_h = dot(normal, halfVector);

	float4 color = ColorTexture.Sample(TextureSampler, IN.TextureCoordinate);
	float specularClamp = SpecularMap.Sample(TextureSampler, IN.TextureCoordinate).x;
	float2 lightCoefficients = lit(n_dot_l, n_dot_h, SpecularPower).yz;

	float3 ambient = color.rgb * AmbientColor;
	float3 diffuse = color.rgb * lightCoefficients.x * LightColor;
	float3 specular = min(lightCoefficients.y, specularClamp) * SpecularColor;

	return float4(lerp(ambient + diffuse + specular, FogColor, IN.FogAmount), sampledAlpha);
}