#include "Engine_Shader_Defines.hlsli"

row_major matrix g_BoneMatrices[MAX_SHADER_BONES];

float4 g_SmearBaseColor = float4(0.05f, 0.08f, 0.25f, 1.f); 
float4 g_SmearEdgeColor = float4(0.45f, 0.65f, 1.f, 1.f); 
float g_SmearAgeRatio = 0.f; 

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    uint4 vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float3 vWorldPos : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 boneMatrix =
        g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
        g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
        g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
        g_BoneMatrices[In.vBlendIndex.w] * In.vBlendWeight.w;

    float4 skinnedPos = mul(float4(In.vPosition, 1.f), boneMatrix);
    float3 skinnedNormal = mul(float4(In.vNormal, 0.f), boneMatrix).xyz;

    float4 worldPos = mul(skinnedPos, g_WorldMatrix);

    Out.vWorldPos = worldPos.xyz;
    Out.vNormal = normalize(mul(float4(skinnedNormal, 0.f), g_WorldMatrix).xyz);
    Out.vPosition = mul(worldPos, g_ViewMatrix);
    Out.vPosition = mul(Out.vPosition, g_ProjMatrix);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float3 vWorldPos : TEXCOORD0;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    float3 viewDir = normalize(g_CamPosition.xyz - In.vWorldPos);
    float rim = 1.f - saturate(dot(normalize(In.vNormal), viewDir));
    rim = pow(rim, 1.8f);

    float4 color = lerp(g_SmearBaseColor, g_SmearEdgeColor, rim);
    color.a *= (1.f - g_SmearAgeRatio);

    Out.vColor = color;
    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_CullNone);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
