float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

vector g_LightDir;
vector g_LightDiffuse;
vector g_LightAmbient;
vector g_LightSpecular;

Texture2D g_DiffuseTexture;

float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
int g_HasDiffuseTexture = 1;

vector g_MtrlAmbiment = vector(0.3f, 0.3f, 0.3f, 1.f);
vector g_MtrlSpecular = vector(1.f, 1.f, 1.f, 1.f);
vector g_CamPosition;

float4 g_OutlineColor = float4(0.1f, 1.f, 0.1f, 1.f);
float g_OutlineThickness = 0.0035f;
int g_IsOutlineEnabled = 0;

sampler DefaultSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = wrap;
    AddressV = wrap;
};

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
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector mtrlDiffuse = g_BaseColorFactor;

    if (g_HasDiffuseTexture != 0)
    {
        mtrlDiffuse *= g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    }

    vector shade = saturate(
        max(dot(normalize(g_LightDir) * -1.f, In.vNormal), 0.f) + (g_LightAmbient * g_MtrlAmbiment));

    vector lookDir = In.vWorldPos - g_CamPosition;
    vector reflectDir = reflect(normalize(g_LightDir), In.vNormal);

    float specular = pow(max(dot(normalize(lookDir) * -1.f, normalize(reflectDir)), 0.f), 50.f);
    vector specularColor = g_LightSpecular * g_MtrlSpecular * specular;

    Out.vColor = g_LightDiffuse * mtrlDiffuse * shade + specularColor;
    Out.vColor.a = 1.f;

    return Out;
}

PS_OUT PS_OUTLINE(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = g_OutlineColor;
    Out.vColor.a = 1.f;
    return Out;
}

BlendState OpaqueBlend
{
    BlendEnable[0] = False;
};

RasterizerState CullBack
{
    FillMode = Solid;
    CullMode = Back;
};

RasterizerState CullFront
{
    FillMode = Solid;
    CullMode = Front;
};

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetBlendState(OpaqueBlend, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetRasterizerState(CullBack);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass OutlinePass
    {
        SetBlendState(OpaqueBlend, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetRasterizerState(CullFront);

        VertexShader = compile vs_5_0 VS_OUTLINE();
        PixelShader = compile ps_5_0 PS_OUTLINE();
    }
}
