struct VS_INPUT
{
    float3 Position        : POSITION0;
    float4 SubPosition     : POSITION1;
    float3 Normal          : NORMAL0;
    float3 Tangent         : TANGENT0;
    float3 Binormal        : BINORMAL0;
    float4 Diffuse         : COLOR0;
    float4 Specular        : COLOR1;
    float2 TexCoords0      : TEXCOORD0;
    float2 TexCoords1      : TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 Position        : POSITION0;
    float2 TexCoords0      : TEXCOORD0;
    float4 ClipPosition    : TEXCOORD1;
    float4 Diffuse         : COLOR0;
};

float4 cfProjectionMatrix[4] : register(c2);
float4 cfViewMatrix[3]       : register(c6);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPosition;
    float4 viewPosition;

    worldPosition = float4(input.Position, 1.0f);

    viewPosition.x = dot(worldPosition, cfViewMatrix[0]);
    viewPosition.y = dot(worldPosition, cfViewMatrix[1]);
    viewPosition.z = dot(worldPosition, cfViewMatrix[2]);
    viewPosition.w = 1.0f;

    output.Position.x = dot(viewPosition, cfProjectionMatrix[0]);
    output.Position.y = dot(viewPosition, cfProjectionMatrix[1]);
    output.Position.z = dot(viewPosition, cfProjectionMatrix[2]);
    output.Position.w = dot(viewPosition, cfProjectionMatrix[3]);

    output.TexCoords0 = input.TexCoords0;
    output.ClipPosition = output.Position;
    output.Diffuse = input.Diffuse;

    return output;
}
