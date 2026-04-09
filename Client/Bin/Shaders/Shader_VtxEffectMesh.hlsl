// Pass 0 : Translucent  (일반 알파 블렌딩)
// Pass 1 : Additive     (가산 블렌딩 — 빛 합산)
// Pass 2 : Opaque       (불투명)
// ==================================================

#include "Engine_Shader_Defines.hlsli"

float2 g_UVOffset;

/* UV 타일링 반복 횟수 */
float2 g_UVTiling;

float4 g_ColorTint;
float  g_Opacity;
int    g_ForceVisiblePreview;

/* 프레넬 강도 (가운데 투명정도) */
float  g_FresnelPower;
float  g_FresnelMultiplier;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal   : NORMAL;
    float3 vTangent  : TANGENT;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV  = mul(g_WorldMatrix, g_ViewMatrix);
    float4x4 matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal   = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix).xyz);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix).xyz;

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    if (g_ForceVisiblePreview != 0)
    {
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
        return Out;
    }

    /* 1. UV 스크롤 적용 — uvScrollSpeed * time이 g_UVOffset으로 들어온다 */
    float2 scrolledUV = In.vTexcoord * g_UVTiling + g_UVOffset;

    /* 2. Diffuse(Emissive) 텍스처 샘플링 */
    float4 diffuse = g_DiffuseTexture.Sample(DefaultSampler, scrolledUV);

    /* 3. 마스크 텍스처 샘플링 — R 채널을 알파로 사용 */
    float maskValue = g_MaskTexture.Sample(DefaultSampler, scrolledUV).r;

    /* 4. 프레넬 (가장자리 투명/발광 효과) */
    float fresnel = 1.0f;
    if (g_FresnelPower > 0.f)
    {
        float3 viewDir = normalize(g_CamPosition.xyz - In.vWorldPos);
        float ndotv = saturate(dot(normalize(In.vNormal), viewDir));
        fresnel = pow(1.0f - ndotv, g_FresnelPower) * g_FresnelMultiplier;
    }

    Out.vColor.rgb = diffuse.rgb * g_ColorTint.rgb;
    Out.vColor.a   = diffuse.a * maskValue * g_Opacity * fresnel * g_ColorTint.a;

    if (Out.vColor.a < 0.01f)
        discard;

    return Out;
}

PS_OUT PS_MAIN_NO_MASK(PS_IN In)
{
    PS_OUT Out;
    if (g_ForceVisiblePreview != 0)
    {
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
        return Out;
    }

    float2 scrolledUV = In.vTexcoord * g_UVTiling + g_UVOffset;
    float4 diffuse = g_DiffuseTexture.Sample(DefaultSampler, scrolledUV);

    float fresnel = 1.0f;
    if (g_FresnelPower > 0.f)
    {
        float3 viewDir = normalize(g_CamPosition.xyz - In.vWorldPos);
        float ndotv = saturate(dot(normalize(In.vNormal), viewDir));
        fresnel = pow(1.0f - ndotv, g_FresnelPower) * g_FresnelMultiplier;
    }

    Out.vColor.rgb = diffuse.rgb * g_ColorTint.rgb;
    Out.vColor.a   = diffuse.a * g_Opacity * fresnel * g_ColorTint.a;

    // 왜안나오냐고 지금
    //if (Out.vColor.a < 0.01f)
    //    discard;

    return Out;
}

technique11 DefaultTechnique
{
    pass TranslucentPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass AdditivePass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OpaquePass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass TranslucentNoMaskPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass AdditiveNoMaskPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass TranslucentTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass AdditiveTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OpaqueTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass TranslucentNoMaskTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }

    pass AdditiveNoMaskTwoSidedPass
    {
        SetRasterizerState(RS_CullNone);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_Additive, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_NO_MASK();
    }
}
