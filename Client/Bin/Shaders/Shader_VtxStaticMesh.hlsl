#include "Engine_Shader_Defines.hlsli"

float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_ShadowColor = float4(1.f, 1.f, 1.f, 1.f);

int g_HasDiffuseTexture = 1;
int g_HasBlendDiffuseTexture = 0;
int g_HasMaskTexture = 0;
int g_HasBlendNormalTexture = 0;
int g_HasUnevenColorTexture = 0;

int g_BaseColorUVChannel = 0;
int g_BlendDiffuseUVChannel = 0;
int g_MaskUVChannel = 0;
int g_BlendNormalUVChannel = 0;
int g_UnevenColorUVChannel = 0;

float g_BaseColorUVScale = 1.f;
float g_BlendDiffuseUVScale = 1.f;
float g_MaskUVScale = 1.f;
float g_BlendNormalUVScale = 1.f;
float g_UnevenColorUVScale = 1.f;

float g_MaskScale = 1.f;
float g_MaskThreshold = 1.f;
float g_BlendNormalStrength = 1.f;
float g_UnevenColorScale = 1.f;

float4 g_OutlineColor = float4(0.1f, 1.f, 0.1f, 1.f);
float g_OutlineThickness = 0.0035f;
int g_IsOutlineEnabled = 0;

Texture2D g_BlendDiffuseTexture;
Texture2D g_BlendNormalTexture;
Texture2D g_UnevenColorTexture;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vWorldPos : TEXCOORD2;
    float4 vProjPos : TEXCOORD3;
    float4 vLightViewPos : TEXCOORD4;
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
    Out.vTexcoord1 = In.vTexcoord1;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    Out.vProjPos = Out.vPosition;
    Out.vLightViewPos = mul(Out.vWorldPos, g_ViewMatrix);

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
    Out.vTexcoord1 = In.vTexcoord1;
    Out.vWorldPos = worldPos;
    Out.vProjPos = Out.vPosition;
    Out.vLightViewPos = mul(worldPos, g_ViewMatrix);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vWorldPos : TEXCOORD2;
    float4 vProjPos : TEXCOORD3;
    float4 vLightViewPos : TEXCOORD4;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal  : SV_TARGET1;
    vector vDepth   : SV_TARGET2;
};

float2 SelectMaterialUV(float2 uv0, float2 uv1, int uvChannel, float uvScale)
{
    float2 selectedUV = (uvChannel == 1) ? uv1 : uv0;
    return selectedUV * uvScale;
}

float ComputeUpFacingMask(float3 worldNormal)
{
    float upDot = saturate(dot(normalize(worldNormal), float3(0.f, 1.f, 0.f)));

    return smoothstep(0.65f, 0.9f, upDot);
}

float ComputeMaskValue(float2 uv0, float2 uv1, float3 worldNormal)
{
    if (g_HasMaskTexture == 0)
        return 0.f;

    float2 maskUV = SelectMaterialUV(uv0, uv1, g_MaskUVChannel, g_MaskUVScale * g_MaskScale);
    float4 maskSample = g_MaskTexture.Sample(DefaultSampler, maskUV);

    float alphaMask = saturate(maskSample.a);
    float inverseRedMask = 1.f - saturate(maskSample.r);
    float useAlpha = step(0.01f, alphaMask) * step(alphaMask, 0.99f);

    float maskValue = inverseRedMask;
    maskValue = lerp(maskValue, alphaMask, useAlpha);

    float maskPower = max(g_MaskThreshold * 3.f, 4.f);
    maskValue = pow(maskValue, maskPower);

    maskValue = smoothstep(0.20f, 0.75f, maskValue);
    maskValue = saturate(maskValue * 2.5f);
    maskValue *= ComputeUpFacingMask(worldNormal);


    return maskValue;
}

vector ComputeLayeredBaseColor(float2 uv0, float2 uv1, float3 worldNormal)
{
    float2 baseUV = SelectMaterialUV(uv0, uv1, g_BaseColorUVChannel, g_BaseColorUVScale);
    vector baseDiffuse = g_BaseColorFactor;

    if (g_HasDiffuseTexture != 0)
        baseDiffuse *= g_DiffuseTexture.Sample(DefaultSampler, baseUV);

    vector finalDiffuse = baseDiffuse;

    if (g_HasBlendDiffuseTexture != 0)
    {
        float2 blendUV = SelectMaterialUV(uv0, uv1, g_BlendDiffuseUVChannel, g_BlendDiffuseUVScale);
        vector blendDiffuse = g_BlendDiffuseTexture.Sample(DefaultSampler, blendUV);
        float maskValue = ComputeMaskValue(uv0, uv1, worldNormal);

        vector softenedBlendDiffuse = lerp(baseDiffuse, blendDiffuse, 0.65f);
        finalDiffuse = lerp(baseDiffuse, softenedBlendDiffuse, maskValue);
    }


    finalDiffuse.a = 1.f;
    return finalDiffuse;
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector mtrlDiffuse = ComputeLayeredBaseColor(In.vTexcoord, In.vTexcoord1, In.vNormal.xyz);

    Out.vDiffuse = vector(mtrlDiffuse.rgb, 1.f);
    Out.vNormal = vector(normalize(In.vNormal.xyz) * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / max(In.vProjPos.w, 0.0001f), In.vProjPos.w, 0.f, 1.f);

    return Out;
}

PS_OUT PS_MAIN_SIMPLE(PS_IN In)
{
    PS_OUT Out;

    float2 baseUV = SelectMaterialUV(In.vTexcoord, In.vTexcoord1, g_BaseColorUVChannel, g_BaseColorUVScale);
    vector baseDiffuse = g_BaseColorFactor;

    if (g_HasDiffuseTexture != 0)
        baseDiffuse *= g_DiffuseTexture.Sample(DefaultSampler, baseUV);

    Out.vDiffuse = vector(baseDiffuse.rgb, 1.f);
    Out.vNormal = vector(normalize(In.vNormal.xyz) * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / max(In.vProjPos.w, 0.0001f), In.vProjPos.w, 0.f, 1.f);

    return Out;
}

PS_OUT PS_OUTLINE(PS_IN In)
{
    PS_OUT Out;

    Out.vDiffuse = g_OutlineColor;
    Out.vDiffuse.a = 1.f;
    Out.vNormal = vector(0.5f, 0.5f, 1.f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / max(In.vProjPos.w, 0.0001f), In.vProjPos.w, 0.f, 1.f);

    return Out;
}

struct PS_OUT_SHADOW
{
    float4 vDepth : SV_TARGET0;
};

PS_OUT_SHADOW PS_MAIN_SHADOW(PS_IN In)
{
    PS_OUT_SHADOW Out;

    float lightDepth = In.vProjPos.z / max(In.vProjPos.w, 0.0001f);
    // clear 색(1,1,1,1)과 실제 shadow 기록 픽셀을 구분하기 위한 alpha 마커다.
    Out.vDepth = float4(lightDepth, In.vProjPos.w / 1000.f, 0.f, 0.f);
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

    pass SimplePass
    {
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetRasterizerState(RS_Default);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_SIMPLE();
    }

    pass ShadowPass
    {
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetRasterizerState(RS_Shadow);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_SHADOW();
    }
}
