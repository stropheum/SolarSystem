cbuffer CBufferPerFrame
{
	float3 AmbientColor;
	float3 LightDirection;
	float3 LightColor;
}

Texture2D ColorTexture;
SamplerState ColorSampler;

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float2 TextureCoordinate : TEXCOORD;
	float3 Normal : NORMAL;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float4 color = ColorTexture.Sample(ColorSampler, IN.TextureCoordinate);
	float3 ambient = color.rgb * AmbientColor;
	float n_dot_l = dot(LightDirection, IN.Normal);
	float3 diffuse = (n_dot_l > 0 ? color.rgb * n_dot_l * LightColor : (float3)0);

	return float4(saturate(ambient + diffuse), color.a);
}