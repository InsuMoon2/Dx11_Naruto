#include "Engine_Shader_Defines.hlsli"

float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
int g_HasDiffuseTexture = 1;

float4 g_OutlineColor = float4(0.1f, 1.f, 0.1f, 1.f);
float g_OutlineThickness = 0.0035f;
int g_IsOutlineEnabled = 0;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);

    return Out;
}

VS_OUT VS_OUTLINE(VS_IN In)
{
    VS_OUT Out;

    float4 worldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    float3 worldNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix)).xyz;

    worldPos.xyz += worldNormal * g_OutlineThickness;

    float4x4 matWV, matWVP;
    matWV = mul(g_ViewMatrix, g_ProjMatrix);

    Out.vPosition = mul(worldPos, mul(g_ViewMatrix, g_ProjMatrix));
    Out.vNormal = float4(worldNormal, 0.f);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = worldPos;

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal  : SV_TARGET1;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector mtrlDiffuse = g_BaseColorFactor;

    if (g_HasDiffuseTexture != 0)
        mtrlDiffuse *= g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);

    Out.vDiffuse = mtrlDiffuse;
    Out.vNormal = vector(normalize(In.vNormal.xyz) * 0.5f + 0.5f, 0.f);

    return Out;
}

PS_OUT PS_OUTLINE(PS_IN In)
{
    PS_OUT Out;

    Out.vDiffuse = g_OutlineColor;
    Out.vDiffuse.a = 1.f;
    Out.vNormal = vector(0.5f, 0.5f, 1.f, 0.f);

    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetRasterizerState(RS_Default);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OutlinePass
    {
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetRasterizerState(RS_CullFront);

        VertexShader = compile vs_5_0 VS_OUTLINE();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_OUTLINE();
    }
}
