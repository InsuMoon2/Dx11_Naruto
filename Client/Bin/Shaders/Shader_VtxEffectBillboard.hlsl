#include "Engine_Shader_Defines.hlsli"

// 이름은 빌보드이긴한데, 나선환 전용이네

Texture2D g_BaseTexture;
Texture2D g_RingTexture;

float4 g_BaseTint;
float4 g_RingTint;
float g_BaseOpacity;
float g_RingOpacity;
int g_UseRing;

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4 worldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    worldPos = mul(worldPos, g_ViewMatrix);
    worldPos = mul(worldPos, g_ProjMatrix);

    Out.vPosition = worldPos;
    Out.vTexcoord = In.vTexcoord;

    return Out;
};

sampler BillboardClampSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = clamp;
    AddressV = clamp;
};

float SampleMask(float4 tex)
{
    if (tex.a > 0.01f)
        return tex.a;

    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.35f) / 0.65f);
}

float BuildRimMask(float radial)
{
    float outer = 1.f - smoothstep(0.68f, 0.92f, radial);
    float inner = 1.f - smoothstep(0.54f, 0.68f, radial);
    return saturate(outer - inner);
}

float4 BuildBaseLayerColor(float4 tex, float2 uv, float4 tint, float opacity)
{
    float texMask = SampleMask(tex);
    float texLum = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));

    float2 centeredUV = uv * 2.f - 1.f;
    float r = length(centeredUV);

    float sphereMask = 1.0f - smoothstep(0.78f, 0.92f, r);
    float baseFill = pow(saturate(1.0f - r / 0.82f), 0.75f);

    float spiralMask = saturate((texMask - 0.38f) / 0.62f);
    spiralMask *= 1.0f - smoothstep(0.62f, 0.80f, r);

    float spiralDetail = saturate((texLum - 0.16f) / 0.84f);
    spiralDetail = pow(spiralDetail, 0.75f) * spiralMask;

    float centerGlow = pow(saturate(1.0f - r / 0.55f), 3.2f);

    float3 baseColor = tint.rgb * baseFill * 1.45f;
    float3 spiralColor = tint.rgb * spiralDetail * 0.42f;
    float3 centerGlowColor = float3(1.0f, 1.0f, 1.0f) * centerGlow * 0.18f;

    float3 finalRgb = baseColor + spiralColor + centerGlowColor;
    float finalAlpha = saturate((baseFill * 1.35f + spiralDetail * 0.30f + centerGlow * 0.12f) * opacity * tint.a);

    finalAlpha *= sphereMask;

    if (finalAlpha < 0.12f)
        discard;

    finalRgb *= finalAlpha;

    float4 outColor;
    outColor.rgb = finalRgb;
    outColor.a = finalAlpha;
    return outColor;
}

float4 BuildRingLayerColor(float4 tex, float2 uv, float4 tint, float opacity)
{
    float texMask = SampleMask(tex);
    float texLum = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));

    float2 centeredUV = uv * 2.f - 1.f;
    float radial = length(centeredUV);

    float rimMask = BuildRimMask(radial);
    float sparkle = saturate((texMask - 0.28f) / 0.72f) * saturate((0.94f - radial) / 0.22f);
    sparkle *= pow(saturate((texLum - 0.20f) / 0.80f), 0.85f);

    float detail = saturate(rimMask * 0.92f + sparkle * 0.55f);
    float alpha = saturate((rimMask * 0.88f + sparkle * 0.25f) * opacity * tint.a);

    if (alpha < 0.02f)
        discard;

    float4 outColor;
    outColor.rgb = tint.rgb * detail * alpha;
    outColor.a = alpha;
    return outColor;
}

float4 PS_MAIN(VS_OUT In) : SV_TARGET0
{
    float4 baseTex = g_BaseTexture.Sample(BillboardClampSampler, In.vTexcoord);
    float4 baseColor = BuildBaseLayerColor(baseTex, In.vTexcoord, g_BaseTint, g_BaseOpacity);

    if (g_UseRing == 0)
        return baseColor;

    float4 ringTex = g_RingTexture.Sample(BillboardClampSampler, In.vTexcoord);
    float4 ringColor = BuildRingLayerColor(ringTex, In.vTexcoord, g_RingTint, g_RingOpacity);

    float4 result;
    result.rgb = baseColor.rgb + ringColor.rgb;
    result.a = saturate(baseColor.a + ringColor.a);

    return result;
}

technique11 DefaultTechnique
{
    pass TranslucentPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass AdditivePass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OpaquePass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
