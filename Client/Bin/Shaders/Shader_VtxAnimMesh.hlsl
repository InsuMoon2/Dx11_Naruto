#include "Engine_Shader_Defines.hlsli"


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

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);

    if (vMtrlDiffuse.a < 0.3f)
        discard;

    vector vShade = saturate(
        max(dot(normalize(g_LightDir) * -1.f, In.vNormal), 0.f) +
        (g_LightAmbient * g_MtrlAmbient));

    vector vLook = In.vWorldPos - g_CamPosition;
    vector vReflect = reflect(normalize(g_LightDir), In.vNormal);

    float fSpecular = pow(
        max(dot(normalize(vLook) * -1.f, normalize(vReflect)), 0.f),
        50.f);

    vector vSpecularColor = g_LightSpecular * g_MtrlSpecular * fSpecular;

    Out.vColor = g_LightDiffuse * vMtrlDiffuse * vShade + vSpecularColor;

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
}
