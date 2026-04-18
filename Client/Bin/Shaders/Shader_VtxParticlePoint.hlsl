#include "Engine_Shader_Defines.hlsli"

Texture2D g_OpacityTexture : register(t2);

float4 g_ColorTint;
float  g_Opacity;
float  g_EmissiveStrength;
int    g_UseLifetimeFade;
int    g_UseFlipbook;
int    g_FlipbookColumns;
int    g_FlipbookRows;
float  g_FlipbookFps;
int    g_FlipbookStartFrame;
int    g_FlipbookEndFrame;
int    g_FlipbookLoop;
int    g_HasMaskTexture;
int    g_HasOpacityTexture;
int    g_BlendMode;
float4 g_CustomParams0;
float4 g_CustomParams1;

sampler PointClampSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = clamp;
    AddressV = clamp;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    row_major float4x4 TransformMatrix : WORLD;
    float2 vLifeTime : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : POSITION;
    float2 vPSize    : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
    float3 vAlignDir : TEXCOORD2;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    vector vPosition = mul(float4(In.vPosition, 1.f), In.TransformMatrix);

    Out.vPosition = mul(vPosition, g_WorldMatrix);
    Out.vPSize = float2(length(In.TransformMatrix._11_12_13), length(In.TransformMatrix._21_22_23));
    Out.vLifeTime = In.vLifeTime;

    Out.vAlignDir = normalize(In.TransformMatrix._31_32_33);

    return Out;
}

struct GS_IN
{
    float4 vPosition : POSITION; 
    float2 vPSize    : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
    float3 vAlignDir : TEXCOORD2;
};

struct GS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

// Point 파티클의 short/long 축 스트레치 값을 customParams에서 읽을 때 0이면 기본 1배를 쓰도록 정리한다.
float ResolvePointStretchScale(float rawValue)
{
    if (abs(rawValue) < 0.0001f)
        return 1.f;

    return max(rawValue, 0.01f);
}

[maxvertexcount(6)]
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];

    float3 toCamera = normalize(g_CamPosition.xyz - In[0].vPosition.xyz);

    float3 cameraRight = cross(float3(0.f, 1.f, 0.f), toCamera);
    if (dot(cameraRight, cameraRight) < 0.0001f)
        cameraRight = cross(float3(1.f, 0.f, 0.f), toCamera);
    cameraRight = normalize(cameraRight);

    float3 cameraUp = normalize(cross(toCamera, cameraRight));

    float3 effectWorldCenter = mul(float4(0.f, 0.f, 0.f, 1.f), g_WorldMatrix).xyz;
    float3 radialDir = In[0].vPosition.xyz - effectWorldCenter;

    float3 worldMoveDir = mul(float4(In[0].vAlignDir, 0.f), g_WorldMatrix).xyz;
    if (dot(worldMoveDir, worldMoveDir) < 0.0001f)
        worldMoveDir = cameraUp;
    else
        worldMoveDir = normalize(worldMoveDir);

    float2 rollDir = float2(dot(radialDir, cameraRight), dot(radialDir, cameraUp));
    if (dot(rollDir, rollDir) < 0.0001f)
        rollDir = float2(dot(worldMoveDir, cameraRight), dot(worldMoveDir, cameraUp));

    if (dot(rollDir, rollDir) < 0.0001f)
        rollDir = float2(0.f, 1.f);
    else
        rollDir = normalize(rollDir);

    // customParams0.x = short axis width scale, customParams0.y = long axis length scale
    const float widthStretch = ResolvePointStretchScale(g_CustomParams0.x);
    const float lengthStretch = ResolvePointStretchScale(g_CustomParams0.y);

    float3 longAxis = normalize(cameraRight * rollDir.x + cameraUp * rollDir.y);
    float3 shortAxis = normalize(cameraRight * rollDir.y - cameraUp * rollDir.x);

    float3 vRight = shortAxis * In[0].vPSize.x * 0.5f * widthStretch;
    float3 vUp = longAxis * In[0].vPSize.y * 0.5f * lengthStretch;

    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);

    Out[0].vPosition = mul(float4(In[0].vPosition.xyz + vRight + vUp, 1.f), matVP);
    Out[0].vTexcoord = float2(0.f, 0.f);
    Out[0].vLifeTime = In[0].vLifeTime;

    Out[1].vPosition = mul(float4(In[0].vPosition.xyz - vRight + vUp, 1.f), matVP);
    Out[1].vTexcoord = float2(1.f, 0.f);
    Out[1].vLifeTime = In[0].vLifeTime;

    Out[2].vPosition = mul(float4(In[0].vPosition.xyz - vRight - vUp, 1.f), matVP);
    Out[2].vTexcoord = float2(1.f, 1.f);
    Out[2].vLifeTime = In[0].vLifeTime;

    Out[3].vPosition = mul(float4(In[0].vPosition.xyz + vRight - vUp, 1.f), matVP);
    Out[3].vTexcoord = float2(0.f, 1.f);
    Out[3].vLifeTime = In[0].vLifeTime;

    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

float SampleParticleMask(float4 tex, bool allowAdditiveFullQuad)
{
    if (tex.a < 0.999f)
        return tex.a;

    if (allowAdditiveFullQuad && g_BlendMode == 1)
        return 1.f;

    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.35f) / 0.65f);
}

float3 ResolveParticleColor(float4 tex)
{
    return tex.rgb;
}

float2 ResolveFlipbookUV(float2 baseUV, float particleAge)
{
    if (g_UseFlipbook == 0)
        return baseUV;

    int columns = max(g_FlipbookColumns, 1);
    int rows = max(g_FlipbookRows, 1);
    int totalFrames = max(columns * rows, 1);
    int startFrame = clamp(g_FlipbookStartFrame, 0, totalFrames - 1);
    int endFrame = g_FlipbookEndFrame;
    if (endFrame < startFrame || endFrame >= totalFrames)
        endFrame = totalFrames - 1;

    int frameCount = max(endFrame - startFrame + 1, 1);
    int relativeFrame = (int)floor(max(particleAge, 0.f) * max(g_FlipbookFps, 0.01f));

    if (g_FlipbookLoop != 0)
        relativeFrame = relativeFrame % frameCount;
    else
        relativeFrame = min(relativeFrame, frameCount - 1);

    int frameIndex = startFrame + relativeFrame;
    int frameX = frameIndex % columns;
    int frameY = frameIndex / columns;

    return float2(
        (baseUV.x + frameX) / columns,
        (baseUV.y + frameY) / rows);
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    float2 finalUV = ResolveFlipbookUV(In.vTexcoord, In.vLifeTime.y);
    float4 tex = g_DiffuseTexture.Sample(PointClampSampler, finalUV);

    float mask = SampleParticleMask(tex, true);

    if (g_HasMaskTexture != 0)
    {
        float4 maskTex = g_MaskTexture.Sample(PointClampSampler, finalUV);
        mask *= SampleParticleMask(maskTex, false);
    }

    if (g_HasOpacityTexture != 0)
    {
        float4 opacityTex = g_OpacityTexture.Sample(PointClampSampler, finalUV);
        mask *= SampleParticleMask(opacityTex, false);
    }

    if (mask < 0.04f)
        discard;

    float lifeRatio = saturate(In.vLifeTime.y / max(In.vLifeTime.x, 0.001f));
    float fade = (g_UseLifetimeFade != 0) ? (1.f - lifeRatio) : 1.f;
    float alpha = saturate(mask * g_Opacity * fade * g_ColorTint.a);
    float3 colorSource = ResolveParticleColor(tex);

    // AlphaBlend 패스가 premultiplied alpha 구성이므로 source rgb에도 alpha를 곱해준다.
    float3 finalRgb = colorSource * g_ColorTint.rgb * max(g_EmissiveStrength, 0.f);
    if (g_BlendMode == 1)
    {
        const bool useAuthoredAlpha = tex.a < 0.999f || g_HasMaskTexture != 0 || g_HasOpacityTexture != 0;
        const float additiveScale = useAuthoredAlpha
            ? alpha
            : saturate(g_Opacity * fade * g_ColorTint.a);

        finalRgb *= additiveScale;
    }

    Out.vColor.rgb = finalRgb;
    Out.vColor.a = alpha;
    Out.vColor.rgb += (g_CustomParams0.rgb + g_CustomParams1.rgb) * 0.f;

    return Out;
}


technique11 DefaultTechnique
{
    pass TranslucentPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader    = compile vs_5_0 VS_MAIN();
        GeometryShader  = compile gs_5_0 GS_MAIN();
        PixelShader     = compile ps_5_0 PS_MAIN();
    }

    pass AdditivePass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader    = compile vs_5_0 VS_MAIN();
        GeometryShader  = compile gs_5_0 GS_MAIN();
        PixelShader     = compile ps_5_0 PS_MAIN();
    }

    pass OpaquePass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader    = compile vs_5_0 VS_MAIN();
        GeometryShader  = compile gs_5_0 GS_MAIN();
        PixelShader     = compile ps_5_0 PS_MAIN();
    }
}
