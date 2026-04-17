#include "Engine_Shader_Defines.hlsli"

float2 g_UVTiling = float2(1.f, 1.f);
float2 g_UVScrollSpeed = float2(0.f, 0.f);
float2 g_SubUVTiling = float2(1.f, 1.f);
float2 g_SubUVScrollSpeed = float2(0.f, 0.f);
float4 g_ColorTint = float4(1.f, 1.f, 1.f, 1.f);
float4 g_HorizonColor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_ZenithColor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_ShadowColor = float4(1.f, 1.f, 1.f, 1.f);

float g_Opacity = 1.f;
float g_EmissiveStrength = 1.f;
float g_ElapsedTime = 0.f;

int g_HasDiffuseTexture = 0;
int g_HasMaskTexture = 0;
int g_RenderStyle = 0;
int g_BaseColorUVChannel = 0;
int g_MaskUVChannel = 0;
float g_BaseColorUVScale = 1.f;
float g_MaskUVScale = 1.f;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal   : NORMAL;
    // `VtxMesh` 입력 레이아웃과 semantic을 맞춰 input layout 생성 실패를 막는다.
    float3 vTangent  : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float3 vLocalDir : TEXCOORD2;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 viewNoTranslation = g_ViewMatrix;
    float4x4 matWV, matWVP;
    float4 clipPosition;

    // Remove camera translation so the sky stays centered on the camera.
    viewNoTranslation._41 = 0.f;
    viewNoTranslation._42 = 0.f;
    viewNoTranslation._43 = 0.f;

    matWV = mul(g_WorldMatrix, viewNoTranslation);
    matWVP = mul(matWV, g_ProjMatrix);

    clipPosition = mul(float4(In.vPosition, 1.f), matWVP);
    // Push the sky to the far plane so near clip does not slice the mesh.
    Out.vPosition = clipPosition.xyww;
    Out.vTexcoord = In.vTexcoord;
    Out.vTexcoord1 = In.vTexcoord1;
    Out.vLocalDir = normalize(In.vPosition);
    
    return Out;
}

float2 SelectMaterialUV(float2 uv0, float2 uv1, int uvChannel, float uvScale, float2 uvTiling, float2 uvScrollSpeed)
{
    float2 selectedUV = (uvChannel == 1) ? uv1 : uv0;
    return selectedUV * uvScale * uvTiling + (uvScrollSpeed * g_ElapsedTime);
}

float4 RenderBaseSky(VS_OUT In)
{
    float horizonLerp = saturate(In.vLocalDir.y * 0.5f + 0.5f);
    horizonLerp = smoothstep(0.05f, 0.95f, horizonLerp);

    float3 skyGradient = lerp(g_HorizonColor.rgb, g_ZenithColor.rgb, horizonLerp);
    float3 skyBands = g_BaseColorFactor.rgb;

    if (g_HasMaskTexture != 0)
    {
        float2 maskUV = SelectMaterialUV(
            In.vTexcoord,
            In.vTexcoord1,
            g_MaskUVChannel,
            g_MaskUVScale,
            g_UVTiling,
            g_UVScrollSpeed);

        float4 maskSample = g_MaskTexture.Sample(DefaultSampler, maskUV);
        float pattern = saturate(maskSample.r);
        skyBands = lerp(g_ShadowColor.rgb, g_BaseColorFactor.rgb, pattern);
    }

    float3 finalRgb = lerp(skyGradient, skyBands, 0.35f);
    finalRgb *= g_ColorTint.rgb;

    return float4(finalRgb * g_EmissiveStrength, g_Opacity);
}

float4 RenderCloudLayer(VS_OUT In)
{
    float baseOpacity = 0.f;
    if (g_HasDiffuseTexture != 0)
    {
        float2 baseUV = SelectMaterialUV(
            In.vTexcoord,
            In.vTexcoord1,
            g_BaseColorUVChannel,
            g_BaseColorUVScale,
            g_UVTiling,
            g_UVScrollSpeed);
        baseOpacity = g_DiffuseTexture.Sample(DefaultSampler, baseUV).r;
    }

    float cloudMask = smoothstep(0.2f, 0.8f, saturate(baseOpacity));

    float3 cloudColor = lerp(g_ShadowColor.rgb, g_BaseColorFactor.rgb, saturate(baseOpacity));
    cloudColor *= g_ColorTint.rgb;

    return float4(cloudColor * g_EmissiveStrength, cloudMask * g_Opacity);
}

float4 RenderDefaultTextured(VS_OUT In)
{
    float2 baseUV = SelectMaterialUV(
        In.vTexcoord,
        In.vTexcoord1,
        g_BaseColorUVChannel,
        g_BaseColorUVScale,
        g_UVTiling,
        g_UVScrollSpeed);

    float4 diffuse = float4(1.f, 1.f, 1.f, 1.f);
    if (g_HasDiffuseTexture != 0)
        diffuse = g_DiffuseTexture.Sample(DefaultSampler, baseUV);

    float maskValue = 1.f;
    if (g_HasMaskTexture != 0)
    {
        float2 maskUV = SelectMaterialUV(
            In.vTexcoord,
            In.vTexcoord1,
            g_MaskUVChannel,
            g_MaskUVScale,
            g_SubUVTiling,
            g_SubUVScrollSpeed);
        maskValue = g_MaskTexture.Sample(DefaultSampler, maskUV).r;
    }

    float4 finalColor = diffuse;
    finalColor.rgb *= g_BaseColorFactor.rgb;
    finalColor.rgb *= g_ColorTint.rgb;
    finalColor.rgb *= g_EmissiveStrength;
    finalColor.a *= g_BaseColorFactor.a;
    finalColor.a *= g_ColorTint.a;
    finalColor.a *= g_Opacity;
    finalColor.a *= maskValue;

    return finalColor;
}

float4 PS_MAIN(VS_OUT In) : SV_TARGET0
{
    float4 finalColor = RenderDefaultTextured(In);

    if (g_RenderStyle == 1)
        finalColor = RenderBaseSky(In);
    else if (g_RenderStyle == 2)
        finalColor = RenderCloudLayer(In);

    if (finalColor.a <= 0.001f)
        discard;

    return finalColor;
}

technique11 DefaultTechnique
{
    pass Opaque_FrontCull
    {
        SetRasterizerState(RS_CullFront);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass Blend_FrontCull
    {
        SetRasterizerState(RS_CullFront);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass Opaque_TwoSided
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass Blend_TwoSided
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}

