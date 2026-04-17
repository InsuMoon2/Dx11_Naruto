#include "Engine_Shader_Defines.hlsli"

Texture2D g_Texture;

// 1차 경계값 -> 이거보다 낮으면 어두운 단계
float g_ToonShadeThreshold0 = 0.30f;
// 2차 경계값 -> 중간 단계
float g_ToonShadeThreshold1 = 0.68f;

float ComputeToonShade(float ndotl)
{
    float lightAmount = saturate(ndotl + (g_LightAmbient.r * g_MtrlAmbient.r));

    if (lightAmount < g_ToonShadeThreshold0)
        return 0.18f;

    if (lightAmount < g_ToonShadeThreshold1)
        return 0.55f;

    return 1.0f;
}

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

struct PS_OUT_BACKBUFFER
{
    vector vColor : SV_TARGET0;
};

PS_OUT_BACKBUFFER PS_MAIN_DEBUG(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;

    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);

    return Out;
}

struct PS_OUT_LIGHT
{
    vector vShade : SV_TARGET0;
};

Texture2D g_NormalTexture;
Texture2D g_ShadeTexture;

PS_OUT_LIGHT PS_MAIN_DIRECTIONAL(PS_IN In)
{
    PS_OUT_LIGHT Out;

    vector vNormalDesc = g_NormalTexture.Sample(DefaultSampler, In.vTexcoord);
    vector vNormal = vector(vNormalDesc.xyz * 2.f - 1.f, 0.f);

    float ndotl = max(dot(normalize(g_LightDir.xyz) * -1.f, normalize(vNormal.xyz)), 0.f);

    // 3단 툰 분리
    float toonShade = ComputeToonShade(ndotl);

    Out.vShade = vector(g_LightDiffuse.rgb * toonShade, 1.f);
    
    return Out;
}

PS_OUT_LIGHT PS_MAIN_POINT(PS_IN In)
{
    PS_OUT_LIGHT Out;
    
    vector vNormalDesc = g_NormalTexture.Sample(DefaultSampler, In.vTexcoord);
    vector vNormal = vector(vNormalDesc.xyz * 2.f - 1.f, 0.f);

    float ndotl = max(dot(normalize(g_LightDir.xyz) * -1.f, normalize(vNormal.xyz)), 0.f);

    float toonShade = ComputeToonShade(ndotl);

    Out.vShade = vector(g_LightDiffuse.rgb * toonShade, 1.f);

    return Out;
}

PS_OUT_BACKBUFFER PS_MAIN_COMBINED(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;
    
    vector vDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    if (vDiffuse.a == 0.f)
        discard;
    
    vector vShade = g_ShadeTexture.Sample(DefaultSampler, In.vTexcoord);
    
    Out.vColor = vDiffuse * vShade;
    
    return Out;
}

technique11 DefaultTechnique
{
    pass Debug
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_DEBUG();
    }

    pass Directional
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_DIRECTIONAL();
    }

    pass Point
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_POINT();
    }

    pass Combined
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_COMBINED();
    }
}
