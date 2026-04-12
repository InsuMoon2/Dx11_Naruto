#include "Engine_Shader_Defines.hlsli"

float4 g_ColorTint;
float  g_Opacity;
int    g_UseLifetimeFade;
int    g_UseFlipbook;
int    g_FlipbookColumns;
int    g_FlipbookRows;
float  g_FlipbookFps;
int    g_FlipbookStartFrame;
int    g_FlipbookEndFrame;
int    g_FlipbookLoop;
float4 g_CustomParams0;
float4 g_CustomParams1;

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
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    vector vPosition = mul(float4(In.vPosition, 1.f), In.TransformMatrix);

    Out.vPosition = mul(vPosition, g_WorldMatrix);
    Out.vPSize = float2(length(In.TransformMatrix._11_12_13), length(In.TransformMatrix._21_22_23));
    Out.vLifeTime = In.vLifeTime;


    return Out;
}

struct GS_IN
{
    float4 vPosition : POSITION;
    float2 vPSize    : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct GS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

[maxvertexcount(6)]
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];
    
    float3 vLook = g_CamPosition.xyz - In[0].vPosition.xyz;
    float3 vRight = normalize(cross(float3(0.f, 1.f, 0.f), vLook)) * In[0].vPSize.x * 0.5f;
    float3 vUp = normalize(cross(vLook, vRight)) * In[0].vPSize.y * 0.5f;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);

    Out[0].vPosition = mul(vector(In[0].vPosition.xyz + vRight + vUp, 1.f), matVP);
    Out[0].vTexcoord = float2(0.f, 0.f);
    Out[0].vLifeTime = In[0].vLifeTime;
    
    Out[1].vPosition = mul(vector(In[0].vPosition.xyz - vRight + vUp, 1.f), matVP);
    Out[1].vTexcoord = float2(1.f, 0.f);
    Out[1].vLifeTime = In[0].vLifeTime;
    
    Out[2].vPosition = mul(vector(In[0].vPosition.xyz - vRight - vUp, 1.f), matVP);
    Out[2].vTexcoord = float2(1.f, 1.f);
    Out[2].vLifeTime = In[0].vLifeTime;
    
    Out[3].vPosition = mul(vector(In[0].vPosition.xyz + vRight - vUp, 1.f), matVP);
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
};

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
    float4 tex = g_DiffuseTexture.Sample(DefaultSampler, finalUV);

    float mask = (tex.a > 0.001f)
        ? tex.a
        : max(tex.r, max(tex.g, tex.b));
    mask = saturate((mask - 0.08f) / 0.92f);

    // 검은/회색 주변부를 더 강하게 제거

    if (mask < 0.02f)
        discard;

    float lifeRatio = saturate(In.vLifeTime.y / max(In.vLifeTime.x, 0.001f));
    float fade = (g_UseLifetimeFade != 0) ? (1.f - lifeRatio) : 1.f;
    float detail = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    detail = max(detail, mask * 0.35f);

    Out.vColor.rgb = g_ColorTint.rgb * detail;
    Out.vColor.a = mask * g_Opacity * fade * g_ColorTint.a;
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
