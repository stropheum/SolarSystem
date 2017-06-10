cbuffer CBufferPerObject
{
	float4x4 WorldViewProjection;
	float4x4 World;
}

struct VS_INPUT
{
	float4 ObjectPosition: POSITION;
	float2 TextureCoordinate : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float3 WorldPosition : WORLDPOS;
	float2 TextureCoordinate : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	OUT.Position = mul(IN.ObjectPosition, WorldViewProjection);
	OUT.WorldPosition = mul(IN.ObjectPosition, World).xyz;
	OUT.TextureCoordinate = IN.TextureCoordinate;
	OUT.Normal = normalize(mul(float4(IN.Normal, 0), World).xyz);
	OUT.Tangent = normalize(mul(float4(IN.Tangent, 0), World).xyz);
	OUT.Binormal = cross(OUT.Normal, OUT.Tangent);

	return OUT;
}
