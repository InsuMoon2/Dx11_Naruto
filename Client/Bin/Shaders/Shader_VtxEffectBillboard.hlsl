#include "Engine_Shader_Defines.hlsli"

Texture2D g_BaseTexture;
Texture2D g_BaseMaskTexture;
Texture2D g_BaseOpacityTexture;
Texture2D g_BaseOpacityGradationTexture;
Texture2D g_RingTexture;
Texture2D g_RingOpacityTexture;
Texture2D g_RingOpacityGradationTexture;
Texture2D g_SceneColorTexture;
Texture2D g_ScreenDistortionNormalTexture;

float4 g_BaseTint;
float4 g_RingTint;
float g_BaseOpacity;
float g_RingOpacity;
float g_BaseEmissiveStrength;
float g_RingEmissiveStrength;
float g_ElapsedTime;
float2 g_BaseUvOffset;
float2 g_BaseUvScale;
float2 g_RingUvOffset;
float2 g_RingUvScale;
float2 g_ScreenDistortionInvViewportSize;
float2 g_ScreenDistortionNormalTiling;
float2 g_ScreenDistortionScrollA;
float2 g_ScreenDistortionScrollB;
float g_ScreenDistortionStrength;
float g_ScreenDistortionRadialStrength;

int g_UseRing;
int g_RenderMode;
int g_HasBaseMaskTexture;
int g_HasBaseOpacityTexture;
int g_HasBaseOpacityGradationTexture;
int g_HasRingOpacityTexture;
int g_HasRingOpacityGradationTexture;
int g_UseBaseFlipbook;
int g_BaseFlipbookColumns;
int g_BaseFlipbookRows;
float g_BaseFlipbookFps;
int g_BaseFlipbookStartFrame;
int g_BaseFlipbookEndFrame;
int g_BaseFlipbookLoop;
int g_UseRingFlipbook;
int g_RingFlipbookColumns;
int g_RingFlipbookRows;
float g_RingFlipbookFps;
int g_RingFlipbookStartFrame;
int g_RingFlipbookEndFrame;
int g_RingFlipbookLoop;
int g_HasScreenDistortionNormalTexture;

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

// Billboard effects mix alpha-authored PNGs and grayscale mask maps,
// so sampling must support both without forcing a single texture convention.
float SampleMask(float4 tex)
{
    if (tex.a < 0.999f)
        return tex.a;

    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.05f) / 0.95f);
}

// Legacy CoreSphere effects used fully opaque texture alpha as "visible",
// so their dark RGB details must not punch holes through the center.
float SampleCoreSphereMask(float4 tex)
{
    if (tex.a > 0.01f)
        return tex.a;

    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.35f) / 0.65f);
}

// CoreSphere uses a radial rim profile so Rasengan-like cores stay soft and rounded.
float BuildRimMask(float radial)
{
    float outer = 1.f - smoothstep(0.68f, 0.92f, radial);
    float inner = 1.f - smoothstep(0.54f, 0.68f, radial);
    return saturate(outer - inner);
}

// Manual UV crop lets atlas textures use one chosen source region without showing the whole sprite sheet.
float2 ApplyManualUvRect(float2 uv, float2 offset, float2 scale)
{
    float2 safeScale = max(scale, float2(0.0001f, 0.0001f));
    return uv * safeScale + offset;
}

// Flipbook billboard layers need deterministic frame addressing inside the selected source UV region.
float2 BuildFlipbookUV(
    float2 uv,
    int enabled,
    int columns,
    int rows,
    float fps,
    int startFrame,
    int endFrame,
    int loopMode,
    float2 sourceOffset,
    float2 sourceScale)
{
    int safeColumns = max(columns, 1);
    int safeRows = max(rows, 1);
    float2 safeSourceScale = max(sourceScale, float2(0.0001f, 0.0001f));

    if (enabled == 0 || (safeColumns == 1 && safeRows == 1))
        return ApplyManualUvRect(uv, sourceOffset, safeSourceScale);

    int frameCount = safeColumns * safeRows;
    int clampedStartFrame = clamp(startFrame, 0, frameCount - 1);
    int resolvedEndFrame = endFrame < 0 ? frameCount - 1 : clamp(endFrame, 0, frameCount - 1);
    int minFrame = min(clampedStartFrame, resolvedEndFrame);
    int maxFrame = max(clampedStartFrame, resolvedEndFrame);
    int span = maxFrame - minFrame + 1;

    float elapsedFrames = max(g_ElapsedTime, 0.f) * max(fps, 0.f);
    int localFrame = 0;

    if (span > 1)
    {
        if (loopMode != 0)
            localFrame = (int) floor(elapsedFrames) % span;
        else
            localFrame = min((int) floor(elapsedFrames), span - 1);
    }

    int frameIndex = minFrame + localFrame;
    int columnIndex = frameIndex % safeColumns;
    int rowIndex = frameIndex / safeColumns;

    float2 tileSize = safeSourceScale / float2(safeColumns, safeRows);
    float2 tileUV = uv * tileSize;
    tileUV += sourceOffset + float2(columnIndex, rowIndex) * tileSize;

    return tileUV;
}

// Opacity helper keeps FlipbookDecal layers compatible with optional mask samples without passing texture objects around.
float ResolveOpacityMask(float4 sampledColor, float opacityMask, float opacityGradationMask)
{
    float maskValue = SampleMask(sampledColor);

    maskValue *= opacityMask;
    maskValue = lerp(maskValue, opacityGradationMask, step(0.0001f, opacityGradationMask));

    return saturate(maskValue);
}

// CoreSphere preserves the old Rasengan-like round core look for existing billboard effects.
float4 BuildCoreSphereBaseColor(float4 tex, float2 uv, float4 tint, float opacity, float emissiveStrength)
{
    float texMask = SampleCoreSphereMask(tex);
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

    float finalAlpha = saturate((baseFill * 1.35f + spiralDetail * 0.30f + centerGlow * 0.12f) * opacity * tint.a);

    float3 finalRgb = (baseColor + spiralColor + centerGlowColor) * emissiveStrength;
    //float3 finalRgb = tint.rgb * finalAlpha * max(emissiveStrength, 0.f);

    finalAlpha *= sphereMask;

    if (finalAlpha < 0.12f)
        discard;

    return float4(finalRgb, finalAlpha);
}

// CoreSphere ring keeps the old outer swirl/rim silhouette used by legacy Rasengan assets.
float4 BuildCoreSphereRingColor(float4 tex, float2 uv, float4 tint, float opacity, float emissiveStrength)
{
    float texMask = SampleCoreSphereMask(tex);
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
    outColor.rgb = tint.rgb * detail * emissiveStrength;
    outColor.a = alpha;
    return outColor;
}

// FlipbookDecal draws sampled texture frames directly instead of wrapping them in a sphere core.
float4 BuildDecalColor(
    float4 colorSample,
    float4 tint,
    float opacity,
    float emissiveStrength,
    float maskSample,
    float opacityMask,
    float opacityGradationMask)
{
    float maskValue = ResolveOpacityMask(
        colorSample,
        opacityMask,
        opacityGradationMask);

    float finalAlpha = saturate(maskValue * maskSample * opacity * tint.a);

    if (finalAlpha < 0.01f)
        discard;

    // Decal-style effect atlases often keep dark RGB values around masked edges,
    // so premultiply by the resolved alpha to prevent black fringe in preview/runtime.
    float3 finalRgb = colorSample.rgb * tint.rgb * emissiveStrength;
    finalRgb *= finalAlpha;
    return float4(finalRgb, finalAlpha);
}

// Distortion billboards treat texture brightness only as a mask; black texels must never become visible color.
float ResolveDistortionMask(
    float4 sampledColor,
    float2 uv,
    float maskSample,
    float opacityMask,
    float opacityGradationMask)
{
    float texMask = ResolveOpacityMask(sampledColor, opacityMask, opacityGradationMask);
    float2 centeredUV = uv * 2.f - 1.f;
    float radial = length(centeredUV);
    float radialSafetyMask = 1.f - smoothstep(0.86f, 1.0f, radial);

    return saturate(texMask * maskSample * radialSafetyMask);
}

// Distortion billboards output only the chosen tint through the resolved mask, never source texture RGB.
float4 BuildDistortionColor(
    float4 sampledColor,
    float2 uv,
    float4 tint,
    float opacity,
    float emissiveStrength,
    float maskSample,
    float opacityMask,
    float opacityGradationMask)
{
    float distortionMask = ResolveDistortionMask(
        sampledColor,
        uv,
        maskSample,
        opacityMask,
        opacityGradationMask);
    float finalAlpha = saturate(distortionMask * opacity * tint.a);

    if (finalAlpha < 0.015f)
        discard;

    float3 finalRgb = tint.rgb * max(emissiveStrength, 0.f);
    return float4(finalRgb, finalAlpha);
}

// ScreenDistortion samples the already-rendered scene and offsets screen UVs inside the billboard mask.
float4 BuildScreenDistortionColor(
    float4 sampledColor,
    float2 uv,
    float4 tint,
    float opacity,
    float maskSample,
    float opacityMask,
    float opacityGradationMask,
    float4 screenPosition)
{
    float distortionMask = ResolveDistortionMask(
        sampledColor,
        uv,
        maskSample,
        opacityMask,
        opacityGradationMask);

    float2 centeredUV = uv * 2.f - 1.f;
    float radial = length(centeredUV);
    float2 radialDir = centeredUV / max(radial, 0.0001f);
    float2 normalOffset = float2(0.f, 0.f);

    if (g_HasScreenDistortionNormalTexture != 0)
    {
        float normalTilingA = max(g_ScreenDistortionNormalTiling.x, 0.0001f);
        float normalTilingB = max(g_ScreenDistortionNormalTiling.y, 0.0001f);
        float2 normalUvA = uv * normalTilingA + g_ScreenDistortionScrollA * g_ElapsedTime;
        float2 normalUvB = uv * normalTilingB + g_ScreenDistortionScrollB * g_ElapsedTime;

        float2 normalA = g_ScreenDistortionNormalTexture.Sample(DefaultSampler, normalUvA).rg * 2.f - 1.f;
        float2 normalB = g_ScreenDistortionNormalTexture.Sample(DefaultSampler, normalUvB).rg * 2.f - 1.f;
        normalOffset = (normalA + normalB) * 0.5f * g_ScreenDistortionStrength;
    }

    float radialFalloff = 1.f - smoothstep(0.15f, 0.95f, radial);
    float2 radialOffset = radialDir * g_ScreenDistortionRadialStrength * radialFalloff;
    float2 screenUV = screenPosition.xy * g_ScreenDistortionInvViewportSize;
    float2 finalOffset = (normalOffset + radialOffset) * distortionMask;
    float finalAlpha = saturate(distortionMask * opacity * tint.a);

    if (finalAlpha < 0.01f)
        discard;

    float3 distortedScene = g_SceneColorTexture.Sample(BillboardClampSampler, saturate(screenUV + finalOffset)).rgb;
    return float4(distortedScene, finalAlpha);
}

float4 PS_MAIN(VS_OUT In) : SV_TARGET0
{
    float2 baseUV = BuildFlipbookUV(
        In.vTexcoord,
        g_UseBaseFlipbook,
        g_BaseFlipbookColumns,
        g_BaseFlipbookRows,
        g_BaseFlipbookFps,
        g_BaseFlipbookStartFrame,
        g_BaseFlipbookEndFrame,
        g_BaseFlipbookLoop,
        g_BaseUvOffset,
        g_BaseUvScale);

    float4 baseTex = g_BaseTexture.Sample(BillboardClampSampler, baseUV);
    float4 baseColor;

    if (g_RenderMode == 1)
    {
        float baseMaskSample = 1.f;
        float baseOpacityMask = 1.f;
        float baseOpacityGradationMask = 0.f;

        if (g_HasBaseMaskTexture != 0)
            baseMaskSample = SampleMask(g_BaseMaskTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityTexture != 0)
            baseOpacityMask = SampleMask(g_BaseOpacityTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityGradationTexture != 0)
            baseOpacityGradationMask = g_BaseOpacityGradationTexture.Sample(
                BillboardClampSampler,
                float2(saturate(SampleMask(baseTex) * baseOpacityMask), 0.5f)).r;

        baseColor = BuildDecalColor(
            baseTex,
            g_BaseTint,
            g_BaseOpacity,
            max(g_BaseEmissiveStrength, 0.f),
            baseMaskSample,
            baseOpacityMask,
            baseOpacityGradationMask);
    }
    else if (g_RenderMode == 2)
    {
        float baseMaskSample = 1.f;
        float baseOpacityMask = 1.f;
        float baseOpacityGradationMask = 0.f;

        if (g_HasBaseMaskTexture != 0)
            baseMaskSample = SampleMask(g_BaseMaskTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityTexture != 0)
            baseOpacityMask = SampleMask(g_BaseOpacityTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityGradationTexture != 0)
            baseOpacityGradationMask = g_BaseOpacityGradationTexture.Sample(
                BillboardClampSampler,
                float2(saturate(SampleMask(baseTex) * baseOpacityMask), 0.5f)).r;

        baseColor = BuildDistortionColor(
            baseTex,
            In.vTexcoord,
            g_BaseTint,
            g_BaseOpacity,
            g_BaseEmissiveStrength,
            baseMaskSample,
            baseOpacityMask,
            baseOpacityGradationMask);
    }
    else if (g_RenderMode == 3)
    {
        float baseMaskSample = 1.f;
        float baseOpacityMask = 1.f;
        float baseOpacityGradationMask = 0.f;

        if (g_HasBaseMaskTexture != 0)
            baseMaskSample = SampleMask(g_BaseMaskTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityTexture != 0)
            baseOpacityMask = SampleMask(g_BaseOpacityTexture.Sample(BillboardClampSampler, baseUV));

        if (g_HasBaseOpacityGradationTexture != 0)
            baseOpacityGradationMask = g_BaseOpacityGradationTexture.Sample(
                BillboardClampSampler,
                float2(saturate(SampleMask(baseTex) * baseOpacityMask), 0.5f)).r;

        baseColor = BuildScreenDistortionColor(
            baseTex,
            In.vTexcoord,
            g_BaseTint,
            g_BaseOpacity,
            baseMaskSample,
            baseOpacityMask,
            baseOpacityGradationMask,
            In.vPosition);
    }
    else
    {
        baseColor = BuildCoreSphereBaseColor(
            baseTex,
            In.vTexcoord,
            g_BaseTint,
            g_BaseOpacity,
            max(g_BaseEmissiveStrength, 0.f));
    }

    if (g_RenderMode == 3 || g_UseRing == 0)
        return baseColor;

    float2 ringUV = BuildFlipbookUV(
        In.vTexcoord,
        g_UseRingFlipbook,
        g_RingFlipbookColumns,
        g_RingFlipbookRows,
        g_RingFlipbookFps,
        g_RingFlipbookStartFrame,
        g_RingFlipbookEndFrame,
        g_RingFlipbookLoop,
        g_RingUvOffset,
        g_RingUvScale);

    float4 ringTex = g_RingTexture.Sample(BillboardClampSampler, ringUV);
    float4 ringColor;

    if (g_RenderMode == 1)
    {
        float ringOpacityMask = 1.f;
        float ringOpacityGradationMask = 0.f;

        if (g_HasRingOpacityTexture != 0)
            ringOpacityMask = SampleMask(g_RingOpacityTexture.Sample(BillboardClampSampler, ringUV));

        if (g_HasRingOpacityGradationTexture != 0)
            ringOpacityGradationMask = g_RingOpacityGradationTexture.Sample(
                BillboardClampSampler,
                float2(saturate(SampleMask(ringTex) * ringOpacityMask), 0.5f)).r;

        ringColor = BuildDecalColor(
            ringTex,
            g_RingTint,
            g_RingOpacity,
            max(g_RingEmissiveStrength, 0.f),
            1.f,
            ringOpacityMask,
            ringOpacityGradationMask);
    }
    else if (g_RenderMode == 2)
    {
        float ringOpacityMask = 1.f;
        float ringOpacityGradationMask = 0.f;

        if (g_HasRingOpacityTexture != 0)
            ringOpacityMask = SampleMask(g_RingOpacityTexture.Sample(BillboardClampSampler, ringUV));

        if (g_HasRingOpacityGradationTexture != 0)
            ringOpacityGradationMask = g_RingOpacityGradationTexture.Sample(
                BillboardClampSampler,
                float2(saturate(SampleMask(ringTex) * ringOpacityMask), 0.5f)).r;

        ringColor = BuildDistortionColor(
            ringTex,
            In.vTexcoord,
            g_RingTint,
            g_RingOpacity,
            g_RingEmissiveStrength,
            1.f,
            ringOpacityMask,
            ringOpacityGradationMask);
    }
    else
    {
        ringColor = BuildCoreSphereRingColor(
            ringTex,
            In.vTexcoord,
            g_RingTint,
            g_RingOpacity,
            max(g_RingEmissiveStrength, 0.f));
    }

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
