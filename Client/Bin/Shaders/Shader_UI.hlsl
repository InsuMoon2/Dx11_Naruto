#include "Engine_Shader_Defines.hlsli"

Texture2D g_Texture;

float g_Alpha = 1.0f;

// 쿨타임
float g_CooldownRatio = 0.f;
float g_CooldownOverlayAlpha = 0.55f;

// 체력바 Fill
float g_FillRatio   = 1.0f;
float g_FillStartU  = 0.0f;
float g_FillEndU    = 1.0f;

// 기본 하얀색
float4 g_BaseColor = float4(1.f, 1.f, 1.f, 1.f);

float g_RotationAngle = 0.0f;

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

float2 Rotate2D(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    return float2(
            pos.x * c - pos.y * s,
            pos.x * s + pos.y * c );
}

VS_OUT VS_ROTATE_UI(VS_IN In)
{
    VS_OUT Out;

    float2 rotatedXY = Rotate2D(In.vPosition.xy, g_RotationAngle);
    float4 localPos = float4(rotatedXY, In.vPosition.z, 1.f);

    float4x4 matWV, matWVP;
    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(localPos, matWVP);
    Out.vTexcoord = In.vTexcoord;

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;    
};

struct PS_OUT
{
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    Out.vColor.a *= g_Alpha;

    if (Out.vColor.a < 0.1f)   
        discard;

    return Out;
}

PS_OUT PS_COLOR(PS_IN In)
{
    PS_OUT Out;

    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    Out.vColor.rgb *= g_BaseColor.rgb; // 컬러 적용
    Out.vColor.a *= g_Alpha;

    if (Out.vColor.a < 0.1f)
        discard;

    return Out;
}

PS_OUT PS_HORIZONTAL_FILL_COLOR(PS_IN In)
{
    PS_OUT Out;

    float4 sampled = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    if (sampled.a < 0.1f)
        discard;

    float fillWidth = g_FillEndU - g_FillStartU;
    float mappedRatioU = g_FillStartU + (fillWidth * g_FillRatio);

    if (In.vTexcoord.x > mappedRatioU)
        discard;

    Out.vColor = sampled * g_BaseColor;
    Out.vColor.a *= g_Alpha;

    return Out;
}

PS_OUT PS_COOLDOWN_OVERLAY(PS_IN In)
{
    PS_OUT Out;

    float4 sampled = g_Texture.Sample(DefaultSampler, In.vTexcoord);

    // 아이콘 투명 영역은 그대로 버림
    if (sampled.a < 0.1f)
        discard;

    float ratio = saturate(g_CooldownRatio);
    if (ratio <= 0.001f)
        discard;

    float cutoffY = 1.f - ratio;

    // 경계 부드러움 범위
    float edgeSoftness = 0.06f;

    // 아래쪽은 1에 가깝고, 경계에서 부드럽게 0으로 감쇠
    float mask = smoothstep(cutoffY - edgeSoftness, cutoffY + edgeSoftness, In.vTexcoord.y);

    float finalAlpha = sampled.a * g_CooldownOverlayAlpha * mask;

    if (finalAlpha < 0.01f)
        discard;

    Out.vColor = float4(0.f, 0.f, 0.f, finalAlpha);

    return Out;
}

PS_OUT PS_MASKED_UI(PS_IN In)
{
    PS_OUT Out;

    float4 sampled = g_Texture.Sample(DefaultSampler, In.vTexcoord);

    float4 maskSample = g_MaskTexture.Sample(DefaultSampler, In.vTexcoord);

    float maskAlpha = step(0.1f, maskSample.a);
    float finalAlpha = sampled.a * maskAlpha * g_Alpha;

    if (finalAlpha < 0.01)
        discard;

    Out.vColor = sampled;
    Out.vColor.a = finalAlpha;

    return Out;
}

PS_OUT PS_MASKED_COOLDOWN_OVERLAY(PS_IN In)
{
    PS_OUT Out;

    float4 sampled = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    float4 maskSample = g_MaskTexture.Sample(DefaultSampler, In.vTexcoord);

    float ratio = saturate(g_CooldownRatio);
    if (ratio <= 0.001f)
        discard;

    // 경계 부드러움 범위
    float edgeSoftness = 0.06f;

    float maskAlpha = step(0.1f, maskSample.a);

    float fillY = 1.f - In.vTexcoord.y;
    float cooldownMask = smoothstep(ratio - edgeSoftness, ratio + edgeSoftness, In.vTexcoord.y);

    float finalAlpha = sampled.a * maskAlpha * g_CooldownOverlayAlpha * cooldownMask;

    if (finalAlpha < 0.01f)
        discard;

    Out.vColor = float4(0.f, 0.f, 0.f, finalAlpha);

    return Out;
}

PS_OUT PS_BLACKKEY_UI(PS_IN In)
{
    PS_OUT Out;

    float4 sampled = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    float brightness = max(sampled.r, max(sampled.g, sampled.b));

    // 검정색 투명취급하게
    if (brightness < 0.05f)
        discard;

    Out.vColor = sampled * g_BaseColor;
    Out.vColor.a = saturate(brightness) * g_Alpha * g_BaseColor.a;

    return Out;
}

PS_OUT PS_SOLID_COLOR(PS_IN In)
{
    PS_OUT Out;

    Out.vColor = g_BaseColor;
    Out.vColor.a *= g_Alpha;

    return Out;
}

technique11 DefaultTechnique
{
    // 0
    pass DefaultPass
    {
        SetDepthStencilState(DSS_None, 0);


        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
    // 1
    pass ColorPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_COLOR();
    }
    // 2
    pass CoolDownOverlayPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_COOLDOWN_OVERLAY();
    }
    // 3
    pass HorizontalFillColorPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_HORIZONTAL_FILL_COLOR();
    }
    // 4
    pass RotationDefaultPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_ROTATE_UI();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
    // 5
    pass MaskedDefaultPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MASKED_UI();
    }
    // 6
    pass MaskedCoolDownOverlayPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MASKED_COOLDOWN_OVERLAY();
    }

    // 7
    pass BlackKeyUIPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_BLACKKEY_UI();
    }

    // 8
    pass SolidColorPass
    {
        SetDepthStencilState(DSS_None, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_SOLID_COLOR();
    }
}

