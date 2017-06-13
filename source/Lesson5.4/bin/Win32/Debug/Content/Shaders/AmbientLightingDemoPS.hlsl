cbuffer CBufferPerFrame
{
	float3 AmbientColor;
}

Texture2D ColorTexture;
SamplerState ColorSampler;

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float2 TextureCoordinate : TEXCOORD;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float4 OUT = (float4)0;

	OUT = ColorTexture.Sample(ColorSampler, IN.TextureCoordinate);
	OUT.rgb *= AmbientColor;

	return OUT;
}