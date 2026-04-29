#include "Engine_Shader_Defines.hlsli"

row_major matrix g_BoneMatrices[MAX_SHADER_BONES];

float4 g_GhostColor = float4(0.02f, 0.02f, 0.02f, 1.f);  // 몸체 색상과 유사하게
float4 g_RimColor = float4(0.1f, 0.4f, 1.f, 1.f);        // 외곽선 색상
float  g_GhostAlpha = 1.f;
float  g_RimPower = 2.f;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    uint4  vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
    float3 vWorldPos : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    /* 4개 본 행렬을 weight로 합쳐 최종 스키닝 행렬 생성 (VtxAnim과 동일 방식) */
    float4x4 BoneMatrix =
        g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
        g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
        g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
        g_BoneMatrices[In.vBlendIndex.w] * In.vBlendWeight.w;

    /* 스키닝 적용 */
    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vSkinnedNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    /* 최종 위치와 월드 위치 */
    Out.vPosition = mul(vSkinnedPosition, matWVP);
    Out.vWorldPos = mul(vSkinnedPosition, g_WorldMatrix).xyz;
    
    Out.vNormal = normalize(mul(float4(vSkinnedNormal, 0.f), g_WorldMatrix).xyz);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
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

    // 림라이트 (외곽일수록 1, 정면일수록 0)
    float rimFactor = 1.f - max(dot(viewDir, normalize(In.vNormal)), 0.f);
    rimFactor = pow(smoothstep(0.f, 1.f, rimFactor), g_RimPower);

    // 색상 섞기
    float4 finalColor = lerp(g_GhostColor, g_RimColor, rimFactor);
    finalColor.a *= g_GhostAlpha;

    Out.vColor = finalColor;

    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_Default);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
