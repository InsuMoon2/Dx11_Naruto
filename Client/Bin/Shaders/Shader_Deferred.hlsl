#include "Engine_Shader_Defines.hlsli"

Texture2D g_Texture;

Texture2D g_NormalTexture;
Texture2D g_ShadeTexture;

Texture2D g_DepthTexture;
Texture2D g_SpecularTexture;
Texture2D g_LightDepthTexture;

SamplerState ShadowPointClampSampler = sampler_state
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = clamp;
    AddressV = clamp;
};

// depth 기반 월드 위치 복원을 위한 inverse matrix
float4x4 g_ViewMatrixInverse;
float4x4 g_ProjMatrixInverse;

// 수업코드 스타일 single shadow light 행렬
float4x4 g_LightViewMatrix;
float4x4 g_LightProjMatrix;

// point light 계산용 데이터
float4 g_LightPos;
float g_LightRange;

// 1차 경계값
float g_ToonShadeThreshold0 = 0.30f;
// 2차 경계값
float g_ToonShadeThreshold1 = 0.68f;

float4 g_PostOutlineColor = float4(0.04f, 0.05f, 0.08f, 1.f);
float2 g_OutlineInvViewportSize = float2(1.f / 1280.f, 1.f / 720.f);
float g_PostOutlineNormalThreshold = 0.32f;
float g_PostOutlineStrength = 0.45f;

// 블러처리
float g_ScreenBlurStrength = 0.f;
float2 g_ScreenBlurDirection = float2(0.f, 1.f);
float2 g_ScreenBlurInvViewportSize = float2(1.f / 1280.f, 1.f / 720.f);

int g_LightCastsShadow = 0;
float g_ShadowBias = 0.0015f;
float g_ShadowStrength = 0.65f;
float g_ShadowSoftness = 1.5f;

float ComputeShadowVisibility(float3 worldPos)
{
    if (g_LightCastsShadow == 0)
        return 1.f;

    float4 lightViewPos = mul(float4(worldPos, 1.f), g_LightViewMatrix);
    float4 lightProjPos = mul(lightViewPos, g_LightProjMatrix);
    float lightInvW = 1.f / max(abs(lightProjPos.w), 0.0001f);
    float3 lightNdc = lightProjPos.xyz * lightInvW;

    float2 shadowUV;
    shadowUV.x = lightNdc.x * 0.5f + 0.5f;
    shadowUV.y = -lightNdc.y * 0.5f + 0.5f;

    if (shadowUV.x < 0.f || shadowUV.x > 1.f || shadowUV.y < 0.f || shadowUV.y > 1.f)
        return 1.f;

    float currentDepth = lightNdc.z;
    if (currentDepth < 0.f || currentDepth > 1.f)
        return 1.f;

    if (lightProjPos.w <= 0.f)
        return 1.f;

    uint shadowWidth = 1u;
    uint shadowHeight = 1u;
    g_LightDepthTexture.GetDimensions(shadowWidth, shadowHeight);
    // shadow RT clear 색은 alpha=1, 실제 기록 픽셀은 alpha=0으로 구분한다.
    const bool useOrthographicShadowDepth = abs(lightProjPos.w - 1.f) <= 0.001f;
    const float currentShadowDepth = useOrthographicShadowDepth ? lightNdc.z : (lightProjPos.w / 1000.f);
    const float depthBias = max(g_ShadowBias, 0.0002f);

    // PCF samples nearby texels so the shadow edge does not expose giant blocky point-sample pixels.
    const float2 shadowTexelSize = 1.f / float2((float)max(shadowWidth, 1u), (float)max(shadowHeight, 1u));
    const float pcfRadius = clamp(g_ShadowSoftness, 0.35f, 2.f);
    float shadowAmount = 0.f;
    float sampleCount = 0.f;

    // Perspective shadow는 수업 레퍼런스처럼 G 채널의 proj.w 기반 깊이를 쓰고,
    // orthographic shadow는 proj.w가 1로 고정되므로 R 채널의 z/w 깊이를 사용한다.
    [unroll]
    for (int offsetY = -1; offsetY <= 1; ++offsetY)
    {
        [unroll]
        for (int offsetX = -1; offsetX <= 1; ++offsetX)
        {
            const float2 sampleUV = shadowUV + float2(offsetX, offsetY) * shadowTexelSize * pcfRadius;
            if (sampleUV.x < 0.f || sampleUV.x > 1.f || sampleUV.y < 0.f || sampleUV.y > 1.f)
                continue;

            const float4 lightDepthDesc = g_LightDepthTexture.SampleLevel(ShadowPointClampSampler, sampleUV, 0.f);
            // shadow RT clear color uses alpha=1, so empty texels should never darken the scene.
            const float storedShadowDepth = useOrthographicShadowDepth ? saturate(lightDepthDesc.x) : max(lightDepthDesc.y, 0.f);
            const float sampleVisible = (lightDepthDesc.w >= 0.999f || (currentShadowDepth - depthBias) <= storedShadowDepth) ? 1.f : 0.f;

            shadowAmount += 1.f - sampleVisible;
            sampleCount += 1.f;
        }
    }
    // 코노하처럼 depth 값이 far plane 근처에 몰려도 hard compare가 눈에 보이도록 bias를 더 직접적으로 사용한다.
    shadowAmount /= max(sampleCount, 1.f);
    return 1.f - shadowAmount * saturate(g_ShadowStrength);
}

float ComputeToonShade(float ndotl)
{
    float lightAmount = saturate(ndotl + (g_LightAmbient.r * g_MtrlAmbient.r));

    if (lightAmount < g_ToonShadeThreshold0)
        return 0.18f;

    if (lightAmount < g_ToonShadeThreshold1)
        return 0.55f;

    return 1.0f;
}

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

// Depth RT에 저장한 clipZ/viewZ로 월드 위치를 복원한다.
// depth.x = clip z/w
// depth.y = view z (원본)
float3 ReconstructWorldPos(float2 uv)
{
    float4 depthDesc = g_DepthTexture.Sample(DefaultSampler, uv);

    float clipZ = depthDesc.x;
    float viewZ = depthDesc.y;

    float4 projPos;
    projPos.x = uv.x * 2.f - 1.f;
    projPos.y = uv.y * -2.f + 1.f;
    projPos.z = clipZ;
    projPos.w = 1.f;

    projPos *= viewZ;

    float4 viewPos = mul(projPos, g_ProjMatrixInverse);
    float4 worldPos = mul(viewPos, g_ViewMatrixInverse);

    return worldPos.xyz / max(worldPos.w, 0.0001f);
}

float3 SampleLitColorClamped(float2 uv)
{
    float2 clampedUV = saturate(uv);

    float4 diffuse = g_DiffuseTexture.Sample(DefaultSampler, clampedUV);
    if (diffuse.a <= 0.001f)
        return 0.f;

    float4 shade = g_ShadeTexture.Sample(DefaultSampler, clampedUV);
    float4 specular = g_SpecularTexture.Sample(DefaultSampler, clampedUV);

    return diffuse.rgb * shade.rgb + specular.rgb;
}

float3 ApplyDirectionalScreenBlur(float2 uv, float3 baseColor)
{
    if (g_ScreenBlurStrength <= 0.001f)
        return baseColor;

    float2 blurDir = g_ScreenBlurDirection;
    float dirLength = length(blurDir);

    if (dirLength <= 0.0001f)
        return baseColor;

    blurDir /= dirLength;

    float blurAmount = saturate(g_ScreenBlurStrength);

    float samplePixelDistance = lerp(7.f, 26.f, blurAmount);
    float2 blurStep = blurDir * g_ScreenBlurInvViewportSize * samplePixelDistance;

    float3 accum = 0.f;
    accum += baseColor * 0.34f;
    accum += SampleLitColorClamped(uv - blurStep * 1.f) * 0.18f;
    accum += SampleLitColorClamped(uv - blurStep * 2.f) * 0.18f;
    accum += SampleLitColorClamped(uv - blurStep * 3.f) * 0.14f;
    accum += SampleLitColorClamped(uv - blurStep * 4.f) * 0.10f;
    accum += SampleLitColorClamped(uv - blurStep * 5.f) * 0.06f;

    return lerp(baseColor, accum, blurAmount);
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
    float4 vColor : SV_TARGET0;
};

PS_OUT_BACKBUFFER PS_MAIN_DEBUG(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;
    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    return Out;
}

struct PS_OUT_LIGHT
{
    float4 vShade : SV_TARGET0;
    float4 vSpecular : SV_TARGET1;
};

PS_OUT_LIGHT PS_MAIN_DIRECTIONAL(PS_IN In)
{
    PS_OUT_LIGHT Out;

    float3 normalWS = DecodeWorldNormal(In.vTexcoord);
    float3 worldPos = ReconstructWorldPos(In.vTexcoord);

    float3 lightDir = normalize(-g_LightDir.xyz);
    float3 viewDir = normalize(g_CamPosition.xyz - worldPos);
    float3 reflectDir = reflect(-lightDir, normalWS);

    float ndotl = max(dot(lightDir, normalWS), 0.f);
    float toonShade = ComputeToonShade(ndotl);
    float spec = pow(max(dot(viewDir, reflectDir), 0.f), 120.f);

    Out.vShade = float4(g_LightDiffuse.rgb * toonShade, 1.f);
    Out.vSpecular = float4(g_LightSpecular.rgb * spec, 0.f);

    return Out;
}

PS_OUT_LIGHT PS_MAIN_POINT(PS_IN In)
{
    PS_OUT_LIGHT Out;
    
    float3 normalWS = DecodeWorldNormal(In.vTexcoord);
    float3 worldPos = ReconstructWorldPos(In.vTexcoord);

    float3 lightVec = g_LightPos.xyz - worldPos;
    float distanceToLight = length(lightVec);
    float3 lightDir = normalize(lightVec);

    float attenuation = saturate((g_LightRange - distanceToLight) / max(g_LightRange, 0.0001f));

    float ndotl = max(dot(lightDir, normalWS), 0.f);
    float toonShade = ComputeToonShade(ndotl);

    float3 viewDir = normalize(g_CamPosition.xyz - worldPos);
    float3 reflectDir = reflect(-lightDir, normalWS);

    float spec = pow(max(dot(viewDir, reflectDir), 0.f), 120.f) * attenuation;

    Out.vShade = float4(g_LightDiffuse.rgb * toonShade * attenuation, 1.f);
    Out.vSpecular = float4(g_LightSpecular.rgb * spec, 0.f);

    return Out;
}

PS_OUT_BACKBUFFER PS_MAIN_COMBINED(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;

    float4 vDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    if (vDiffuse.a == 0.f)
        discard;

    float4 vShade = g_ShadeTexture.Sample(DefaultSampler, In.vTexcoord);
    float4 vSpecular = g_SpecularTexture.Sample(DefaultSampler, In.vTexcoord);
    float3 worldPos = ReconstructWorldPos(In.vTexcoord);
    float shadowVisibility = ComputeShadowVisibility(worldPos);

    float3 litColor = (vDiffuse.rgb * vShade.rgb + vSpecular.rgb * 0.2f) * shadowVisibility;
    float3 blurredLitColor = ApplyDirectionalScreenBlur(In.vTexcoord, litColor);

    float outlineMask = ComputePostOutlineMask(In.vTexcoord);

    Out.vColor = float4(lerp(blurredLitColor, g_PostOutlineColor.rgb, outlineMask), 1.f);

    return Out;
}

BlendState BS_LightAccumulate
{
    BlendEnable[0] = true;
    SrcBlend[0] = One;
    DestBlend[0] = One;
    BlendOp[0] = Add;
    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = One;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 0x0F;

    BlendEnable[1] = true;
    SrcBlend[1] = One;
    DestBlend[1] = One;
    BlendOp[1] = Add;
    SrcBlendAlpha[1] = One;
    DestBlendAlpha[1] = One;
    BlendOpAlpha[1] = Add;
    RenderTargetWriteMask[1] = 0x0F;
};

technique11 DefaultTechnique
{
    pass Debug
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_DEBUG();
    }

    pass Directional
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_LightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN_DIRECTIONAL();
    }

    pass Point
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_None, 0);
        SetBlendState(BS_LightAccumulate, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

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
