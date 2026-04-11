// Pass 0 : Translucent  (일반 알파 블렌딩)
// Pass 1 : Additive     (가산 블렌딩 — 빛 합산)
// Pass 2 : Opaque       (불투명)
// ==================================================

#include "Engine_Shader_Defines.hlsli"

Texture2D g_EmissiveTexture;
Texture2D g_OpacityTexture;
Texture2D g_OpacitySubUvTexture;
Texture2D g_OpacityGradationTexture;
Texture2D g_EmissiveGradationTexture;
Texture2D g_UVDistortionTexture;

float2 g_UVOffset;
float2 g_UVDistortionOffset;

float2 g_UVTiling;
float2 g_UVDistortionStrength;

float4 g_ColorTint;
float g_Opacity;
int g_ForceVisiblePreview;

int g_HasOpacityTexture;
int g_HasOpacitySubUvTexture;
int g_HasOpacityGradationTexture;
int g_HasEmissiveGradationTexture;
int g_HasUVDistortionTexture;

/* 프레넬 강도 (가운데 투명정도) */
float  g_FresnelPower;
float  g_FresnelMultiplier;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal   : NORMAL;
    float3 vTangent  : TANGENT;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV  = mul(g_WorldMatrix, g_ViewMatrix);
    float4x4 matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal   = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix).xyz);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix).xyz;


    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

float2 BuildFinalUV(float2 baseUV)
{
    float2 finalUV = baseUV * g_UVTiling + g_UVOffset;

    if (g_HasUVDistortionTexture != 0)
    {
        float2 distortionUV = baseUV * g_UVTiling + g_UVDistortionOffset;
        float2 distortionSample = g_UVDistortionTexture.Sample(DefaultSampler, distortionUV).rg * 2.f - 1.f;
        finalUV += distortionSample * g_UVDistortionStrength;
    }

    return finalUV;
}

float3 ApplyEmissiveGradation(float3 emissiveRgb)
{
    float3 result = emissiveRgb;

    if (g_HasEmissiveGradationTexture != 0)
    {
        float emissiveMask = max(emissiveRgb.r, max(emissiveRgb.g, emissiveRgb.b));
        float3 gradation = g_EmissiveGradationTexture.Sample(DefaultSampler, float2(saturate(emissiveMask), 0.5f)).rgb;
        result *= gradation;
    }

    return result;
}

float ApplyOpacityPipeline(float2 uv)
{
    float maskValue = 1.f;

    if (g_HasOpacityTexture != 0)
    {
        maskValue = g_OpacityTexture.Sample(DefaultSampler, uv).r;
    }

    if (g_HasOpacitySubUvTexture != 0)
    {
        float subMaskValue = g_OpacitySubUvTexture.Sample(DefaultSampler, uv).r;
        maskValue = saturate(maskValue * subMaskValue);
    }

    if (g_HasOpacityGradationTexture != 0)
    {
        float gradMask = g_OpacityGradationTexture.Sample(
            DefaultSampler, float2(saturate(maskValue), 0.5f)).r;

        maskValue = gradMask;
    }

    return saturate(maskValue);
}



float ResolveFresnel(float3 worldNormal, float3 worldPos)
{
    float fresnel = 1.0f;

    if (g_FresnelPower > 0.f)
    {
        float3 viewDir = normalize(g_CamPosition.xyz - worldPos);
        float ndotv = saturate(dot(normalize(worldNormal), viewDir));
        fresnel = pow(1.0f - ndotv, g_FresnelPower) * g_FresnelMultiplier;
    }

    return fresnel;
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    if (g_ForceVisiblePreview != 0)
    {
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
        return Out;
    }

    float2 scrolledUV = BuildFinalUV(In.vTexcoord);

    float4 emissive = g_EmissiveTexture.Sample(DefaultSampler, scrolledUV);
    emissive.rgb = ApplyEmissiveGradation(emissive.rgb);

    float maskValue = ApplyOpacityPipeline(scrolledUV);
    float fresnel = ResolveFresnel(In.vNormal, In.vWorldPos);

    float finalAlpha = saturate(maskValue * g_Opacity * fresnel * g_ColorTint.a);

    if (finalAlpha < 0.01f)
        discard;

    Out.vColor.rgb = emissive.rgb * g_ColorTint.rgb;
    Out.vColor.a = finalAlpha;

    return Out;
}


PS_OUT PS_MAIN_NO_MASK(PS_IN In)
{
    PS_OUT Out;

    if (g_ForceVisiblePreview != 0)
    {
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
        return Out;
    }

    float2 scrolledUV = BuildFinalUV(In.vTexcoord);

    float4 emissive = g_EmissiveTexture.Sample(DefaultSampler, scrolledUV);
    emissive.rgb = ApplyEmissiveGradation(emissive.rgb);

    float fresnel = ResolveFresnel(In.vNormal, In.vWorldPos);
    float emissiveAlpha = max(emissive.a, max(emissive.r, max(emissive.g, emissive.b)));
    float finalAlpha = saturate(emissiveAlpha * g_Opacity * fresnel * g_ColorTint.a);

    if (finalAlpha < 0.01f)
        discard;

    Out.vColor.rgb = emissive.rgb * g_ColorTint.rgb;
    Out.vColor.a = finalAlpha;
    return Out;
}


technique11 DefaultTechnique
{
    pass TranslucentPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass AdditivePass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OpaquePass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass TranslucentNoMaskPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass AdditiveNoMaskPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass TranslucentTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass AdditiveTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OpaqueTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass TranslucentNoMaskTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass AdditiveNoMaskTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }
}
