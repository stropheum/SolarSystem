cbuffer CBufferPerFrame
{
	float3 CameraPosition;
}

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
};

struct VS_OUTPUT
{
	float4 Position: SV_Position;
	float2 TextureCoordinate : TEXCOORD;
	float3 ReflectionVector : REFLECTION;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	OUT.Position = mul(IN.ObjectPosition, WorldViewProjection);
	OUT.TextureCoordinate = IN.TextureCoordinate;
	
	float3 worldPosition = mul(IN.ObjectPosition, World).xyz;
	float3 incident = normalize(worldPosition - CameraPosition);
	float3 normal = normalize(mul(float4(IN.Normal, 0), World).xyz);

	// Reflection Vector for cube map: R = I - 2*N * (I.N)
	OUT.ReflectionVector = reflect(incident, normal);

	return OUT;
}
