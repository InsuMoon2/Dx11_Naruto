#include "Engine_Shader_Defines.hlsli"

Texture2D g_Texture;
Texture2D g_NormalTexture;
Texture2D g_ShadeTexture;

// 1차 경계값 -> 이거보다 낮으면 어두운 단계
float g_ToonShadeThreshold0 = 0.30f;
// 2차 경계값 -> 중간 단계
float g_ToonShadeThreshold1 = 0.68f;

float4 g_PostOutlineColor = float4(0.04f, 0.05f, 0.08f, 1.f);
// 화면 해상도 역수. 1픽셀 옆 샘플링에 사용한다.
float2 g_OutlineInvViewportSize = float2(1.f / 1280.f, 1.f / 720.f);
float g_PostOutlineNormalThreshold = 0.32f;
float g_PostOutlineStrength = 0.45f;

float ComputeToonShade(float ndotl)
{
    float lightAmount = saturate(ndotl + (g_LightAmbient.r * g_MtrlAmbient.r));

    if (lightAmount < g_ToonShadeThreshold0)
        return 0.18f;

    if (lightAmount < g_ToonShadeThreshold1)
        return 0.55f;

    return 1.0f;
}

// GBUffer 노멀을 실제 노멀 범위로 복원하기
float3 DecodeWorldNormal(float2 uv)
{
    float3 encoded = g_NormalTexture.Sample(DefaultSampler, uv).xyz;
    return normalize(encoded * 2.f - 1.f);
}

float ComputePostOutlineMask(float2 uv)
{
    float2 texel = g_OutlineInvViewportSize;

    float3 centerN = DecodeWorldNormal(uv);
    float edge = 0.f;

    edge = max(edge, distance(centerN, DecodeWorldNormal(uv + float2(texel.x, 0.f))));
    edge = max(edge, distance(centerN, DecodeWorldNormal(uv + float2(-texel.x, 0.f))));
    edge = max(edge, distance(centerN, DecodeWorldNormal(uv + float2(0.f, texel.y))));
    edge = max(edge, distance(centerN, DecodeWorldNormal(uv + float2(0.f, -texel.y))));

    float mask = smoothstep(
        g_PostOutlineNormalThreshold,
        g_PostOutlineNormalThreshold + 0.08f,
        edge);

    return mask * g_PostOutlineStrength;
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

    vector toonColor = vDiffuse * vShade;

    float outlineMask = ComputePostOutlineMask(In.vTexcoord);

    Out.vColor = lerp(toonColor, g_PostOutlineColor, outlineMask);
    Out.vColor.a = 1.f;
    
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
