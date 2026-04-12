// Pass 0 : Translucent
// Pass 1 : Additive
// Pass 2 : Opaque
// Pass 3 : TranslucentNoMask
// Pass 4 : AdditiveNoMask
// Pass 5 : TranslucentTwoSided
// Pass 6 : AdditiveTwoSided
// Pass 7 : OpaqueTwoSided
// Pass 8 : TranslucentNoMaskTwoSided
// Pass 9 : AdditiveNoMaskTwoSided

#include "Engine_Shader_Defines.hlsli"

Texture2D g_EmissiveTexture;
Texture2D g_OpacityTexture;
Texture2D g_OpacitySubUvTexture;
Texture2D g_OpacityGradationTexture;
Texture2D g_EmissiveGradationTexture;
Texture2D g_UVDistortionTexture;
Texture2D g_NormalTexture;
Texture2D g_RoughnessTexture;
Texture2D g_SpecularTexture;

float2 g_UVOffset;
float2 g_UVDistortionOffset;
float2 g_UVTiling;
float2 g_UVDistortionStrength;

float4 g_ColorTint;
float g_Opacity;
float g_EmissiveStrength;
float g_FresnelPower;
float g_FresnelMultiplier;

float g_NormalStrength;
float g_Roughness;
float g_SpecularStrength;
float g_SpecularPower;
float4 g_CustomParams0;
float4 g_CustomParams1;

int g_ForceVisiblePreview;
int g_ShadingMode;
int g_HasDiffuseTexture;
int g_HasOpacityTexture;
int g_HasOpacitySubUvTexture;
int g_HasOpacityGradationTexture;
int g_HasEmissiveGradationTexture;
int g_HasUVDistortionTexture;
int g_HasNormalTexture;
int g_HasRoughnessTexture;
int g_HasSpecularTexture;

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
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4 worldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    float4 viewPos = mul(worldPos, g_ViewMatrix);

    Out.vPosition = mul(viewPos, g_ProjMatrix);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix).xyz);
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix).xyz);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = worldPos.xyz;

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

// Mesh effect layers should resolve mask textures the same way billboards do:
// prefer alpha when it exists, otherwise derive a stable mask from luminance.
float SampleMask(float4 tex)
{
    if (tex.a > 0.01f)
        return tex.a;

    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.35f) / 0.65f);
}

// Emissive-only effect maps in this project often keep opaque alpha on a black background.
// For no-mask passes we should derive visibility from RGB brightness, not the source alpha.
float SampleEmissiveMask(float4 tex)
{
    float luminance = dot(tex.rgb, float3(0.299f, 0.587f, 0.114f));
    return saturate((luminance - 0.05f) / 0.95f);
}

// Applies tiling, scroll, and optional distortion so every texture slot samples the same final UV.
float2 BuildFinalUV(float2 uv)
{
    float2 finalUV = uv * g_UVTiling + g_UVOffset;

    if (g_HasUVDistortionTexture != 0)
    {
        float2 distortionUV = uv * g_UVTiling + g_UVDistortionOffset;
        float2 distortionSample = g_UVDistortionTexture.Sample(DefaultSampler, distortionUV).rg * 2.f - 1.f;
        finalUV += distortionSample * g_UVDistortionStrength;
    }

    return finalUV;
}

// Uses a normal map only for Lit mesh layers; Unlit effects keep their mesh normal.
float3 ResolveSurfaceNormal(PS_IN In, float2 uv)
{
    float3 normal = normalize(In.vNormal);

    if (g_ShadingMode == 1 && g_HasNormalTexture != 0)
    {
        float3 tangent = normalize(In.vTangent);
        float3 bitangent = normalize(cross(normal, tangent));
        float3 normalSample = g_NormalTexture.Sample(DefaultSampler, uv).xyz * 2.f - 1.f;
        normalSample.xy *= g_NormalStrength;
        normal = normalize(normalSample.x * tangent + normalSample.y * bitangent + normalSample.z * normal);
    }

    return normal;
}

// Remaps emissive intensity with a gradation texture when the effect asset provides one.
float3 ApplyEmissiveGradation(float3 emissiveRgb)
{
    float3 result = emissiveRgb;

    if (g_HasEmissiveGradationTexture != 0)
    {
        float emissiveMask = max(emissiveRgb.r, max(emissiveRgb.g, emissiveRgb.b));
        float3 gradation = g_EmissiveGradationTexture.Sample(DefaultSampler, float2(saturate(emissiveMask), 0.5f)).rgb;
        result *= gradation;
    }

    return result;
}

// Fresnel is optional; zero power means "do not affect alpha".
float ResolveFresnel(float3 normal, float3 worldPos)
{
    float fresnel = 1.f;

    if (g_FresnelPower > 0.f)
    {
        float3 viewDir = normalize(g_CamPosition.xyz - worldPos);
        float ndotv = saturate(dot(normalize(normal), viewDir));
        fresnel = pow(1.f - ndotv, g_FresnelPower) * g_FresnelMultiplier;
    }

    return saturate(fresnel);
}

// Builds the final alpha mask from explicit opacity, sub-UV opacity, and gradation slots.
float ApplyOpacityPipeline(float2 uv)
{
    float maskValue = 1.f;

    if (g_HasOpacityTexture != 0)
    {
        maskValue = SampleMask(g_OpacityTexture.Sample(DefaultSampler, uv));
    }

    if (g_HasOpacitySubUvTexture != 0)
    {
        float subMaskValue = SampleMask(g_OpacitySubUvTexture.Sample(DefaultSampler, uv));
        maskValue = saturate(maskValue * subMaskValue);
    }

    if (g_HasOpacityGradationTexture != 0)
    {
        float gradMask = g_OpacityGradationTexture.Sample(DefaultSampler, float2(saturate(maskValue), 0.5f)).r;
        maskValue = gradMask;
    }

    return saturate(maskValue);
}

// Roughness texture is optional; this keeps Lit mode stable even when only scalar roughness exists.
float ResolveSurfaceRoughness(float2 uv)
{
    float roughness = g_Roughness;

    if (g_HasRoughnessTexture != 0)
    {
        roughness *= g_RoughnessTexture.Sample(DefaultSampler, uv).r;
    }

    return saturate(roughness);
}

// Specular mask is optional and multiplies the scalar strength from the effect asset.
float ResolveSpecularMask(float2 uv)
{
    float specularMask = 1.f;

    if (g_HasSpecularTexture != 0)
    {
        specularMask = g_SpecularTexture.Sample(DefaultSampler, uv).r;
    }

    return saturate(specularMask);
}

// Lit mode adds simple directional diffuse/specular while Unlit skips this path entirely.
float3 BuildLitColor(float2 uv, float3 normal, float3 worldPos, float3 baseColor, float3 emissive)
{
    float3 lightDir = normalize(-g_LightDir.xyz);
    float3 viewDir = normalize(g_CamPosition.xyz - worldPos);
    float3 halfDir = normalize(lightDir + viewDir);
    float diffuse = saturate(dot(normal, lightDir));

    float roughness = ResolveSurfaceRoughness(uv);
    float specPower = lerp(g_SpecularPower, 4.f, roughness);
    float specular = pow(saturate(dot(normal, halfDir)), specPower);
    specular *= g_SpecularStrength * ResolveSpecularMask(uv);

    float3 litColor = baseColor * (g_LightAmbient.rgb + g_LightDiffuse.rgb * diffuse);
    litColor += g_LightSpecular.rgb * specular;
    litColor += emissive;

    return litColor;
}

// Shared pixel path for masked and no-mask passes so C++ pass indices keep identical behavior.
PS_OUT ResolvePixel(PS_IN In, bool useOpacityMask)
{
    PS_OUT Out;
    Out.vColor = float4(0.f, 0.f, 0.f, 0.f);

    if (g_ForceVisiblePreview != 0)
    {
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
        return Out;
    }

    float2 scrolledUV = BuildFinalUV(In.vTexcoord);

    float4 emissive = g_EmissiveTexture.Sample(DefaultSampler, scrolledUV);
    emissive.rgb = ApplyEmissiveGradation(emissive.rgb);
    emissive.rgb *= g_EmissiveStrength;

    float3 surfaceNormal = ResolveSurfaceNormal(In, scrolledUV);
    float fresnel = ResolveFresnel(surfaceNormal, In.vWorldPos);
    float emissiveAlpha = SampleEmissiveMask(emissive);
    float maskValue = useOpacityMask ? ApplyOpacityPipeline(scrolledUV) : emissiveAlpha;

    float finalAlpha = saturate(maskValue * g_Opacity * fresnel * g_ColorTint.a);
    
    // Diagnostic Override: Force visibility to 0.5 min alpha
    finalAlpha = saturate(finalAlpha + 0.5f); 

    float3 baseColor = emissive.rgb;
    if (g_HasDiffuseTexture != 0)
    {
        baseColor = g_DiffuseTexture.Sample(DefaultSampler, scrolledUV).rgb;
    }

    baseColor *= g_ColorTint.rgb;

    float3 finalColor = emissive.rgb * g_ColorTint.rgb;

    if (g_ShadingMode == 1)
    {
        finalColor = BuildLitColor(scrolledUV, surfaceNormal, In.vWorldPos, baseColor, emissive.rgb * g_ColorTint.rgb);
    }

    // Pre-multiplied alpha (Multiply RGB by Alpha)
    finalColor *= finalAlpha;

    Out.vColor.rgb = finalColor;
    Out.vColor.a = finalAlpha;
    Out.vColor.rgb += (g_CustomParams0.rgb + g_CustomParams1.rgb) * 0.f;

    return Out;
}

PS_OUT PS_MAIN(PS_IN In)
{
    return ResolvePixel(In, true);
}

PS_OUT PS_MAIN_NO_MASK(PS_IN In)
{
    return ResolvePixel(In, false);
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
