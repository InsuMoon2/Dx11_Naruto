#include "Engine_Shader_Defines.hlsli"

float4 g_OutlineColor = float4(0.04f, 0.05f, 0.08f, 1.f);
float g_OutlineThickness = 0.0035f;

row_major matrix g_BoneMatrices[512];

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
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    /* 4개 본 행렬을 weight로 합쳐 최종 스키닝 행렬 생성 */
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

    /* 최종 clip space 위치 */
    Out.vPosition = mul(vSkinnedPosition, matWVP);

    /* 스키닝된 노말을 월드 공간으로 변환 */
    Out.vNormal = normalize(mul(float4(vSkinnedNormal, 0.f), g_WorldMatrix));

    Out.vTexcoord = In.vTexcoord;

    /* 월드 위치도 스키닝 적용된 정점 기준으로 계산 */
    Out.vWorldPos = mul(vSkinnedPosition, g_WorldMatrix);

    return Out;
}

VS_OUT VS_OUTLINE(VS_IN In)
{
    VS_OUT Out;

    float4x4 BoneMatrix =
        g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
        g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
        g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
        g_BoneMatrices[In.vBlendIndex.w] * In.vBlendWeight.w;

    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vSkinnedNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;

    float4 worldPos = mul(vSkinnedPosition, g_WorldMatrix);
    float3 worldNormal = normalize(mul(float4(vSkinnedNormal, 0.f), g_WorldMatrix)).xyz;

    worldPos.xyz += worldNormal * g_OutlineThickness;

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

PS_OUT PS_OUTLINE(PS_IN In)
{
    PS_OUT Out;

    Out.vDiffuse = g_OutlineColor;
    Out.vDiffuse.a = 1.f;
    Out.vNormal = vector(0.5f, 0.5f, 1.f, 0.f);

    return Out;
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);

    if (vMtrlDiffuse.a < 0.3f)
        discard;

    Out.vDiffuse = vMtrlDiffuse;
    Out.vNormal = vector(normalize(In.vNormal.xyz) * 0.5f + 0.5f, 0.f);

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
