#include "Engine_Shader_Defines.hlsli"

Texture2D g_Texture;

float4  g_TintColor;
float   g_EmissiveStrength;
float   g_UVScrollX;

float   g_Time;

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
    
    float4x4 matWV, matWVP;
    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP); 
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;    
};

struct PS_OUT
{
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    float2 uv = In.vTexcoord;
    uv.x += g_Time * g_UVScrollX;

    float rawMask = g_Texture.Sample(DefaultSampler, uv).r;
    float flowMask = smoothstep(0.62f, 0.92f, rawMask);
    float width01 = 1.f - abs(In.vTexcoord.y * 2.f - 1.f);

    float vFade = smoothstep(0.f, 0.25f, width01);

    float glowMask = flowMask * pow(width01, 1.8f) * vFade;
    float coreMask = flowMask * pow(width01, 6.0f) * vFade;

    float3 glowColor = g_TintColor.rgb * glowMask * (g_EmissiveStrength * 0.18f);
    float3 coreColor = g_TintColor.rgb * coreMask * (g_EmissiveStrength * 0.55f);

    Out.vColor = float4(glowColor + coreColor, 1.f);

    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetRasterizerState(RS_CullNone);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}

