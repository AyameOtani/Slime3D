sampler SlimeTexture      : register(s0);
sampler BackgroundTexture : register(s1);
sampler FlowTexture        : register(s2);

float4 EffectParam : register(c6);

struct PS_INPUT
{
    float2 TexCoords0   : TEXCOORD0;
    float4 ClipPosition : TEXCOORD1;
    float4 Diffuse      : COLOR0;
};

float4 main(PS_INPUT input) : COLOR0
{
    float invW;
    float2 ndc;
    float2 screenUV;
    float2 flowUV;
    float2 flow;
    float2 distortion;
    float2 refractUV;
    float4 backgroundColor;
    float4 slimeColor;
    float3 finalColor;
    float3 tintColor;
    float tintAmount;

    invW = 1.0f / max(abs(input.ClipPosition.w), 0.0001f);

    ndc = input.ClipPosition.xy * invW;

    screenUV.x = ndc.x * 0.5f + 0.5f;
    screenUV.y = -ndc.y * 0.5f + 0.5f;

    flowUV = input.TexCoords0 * 2.0f;

    // FlowTextureÇècï˚å¸Ç÷ÉXÉNÉçÅ[Éã
    flowUV.y += EffectParam.z;

    flow = tex2D(FlowTexture, flowUV).rg;
    flow = flow * 2.0f - 1.0f;

    distortion = flow * EffectParam.y;

    refractUV = saturate(screenUV + distortion);

    backgroundColor = tex2D(BackgroundTexture, refractUV);
    slimeColor = tex2D(SlimeTexture, input.TexCoords0);

    finalColor = lerp( backgroundColor.rgb,slimeColor.rgb * input.Diffuse.rgb, 0.35f );

    tintColor = float3(0.15f, 1.0f, 0.45f);
    tintAmount = saturate(EffectParam.w);

    finalColor = lerp( finalColor, finalColor * tintColor + tintColor * 0.10f, tintAmount );

    return float4(saturate(finalColor), 1.0f);
}
