cbuffer CBufferPerObject
{
	float4x4 WorldViewProjection;
	float4x4 World;
};

struct VS_INPUT
{
	float4 ObjectPosition : POSITION;
	float2 TextureCoordinates : TEXCOORD;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TextureCoordinates : TEXCOORD;
	float3 Normal : NORMAL;
	float3 WorldPosition : WORLDPOS;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	OUT.Position = mul(IN.ObjectPosition, WorldViewProjection);
	OUT.TextureCoordinates = IN.TextureCoordinates;
	OUT.Normal = normalize(mul(float4(IN.Normal, 0), World).xyz);
	OUT.WorldPosition = mul(IN.ObjectPosition, World).xyz;

	return OUT;
}