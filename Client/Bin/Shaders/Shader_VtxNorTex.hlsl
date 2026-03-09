
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

vector g_LightDir;
vector g_LightDiffuse;
vector g_LightAmbient;
vector g_LightSpecular;

Texture2D g_DiffuseTexture;
vector g_MtrlAmbiment = vector(0.3f, 0.3f, 0.3, 1.f);
vector g_MtrlSpecular = vector(1.f, 1.f, 1.f, 1.f);
vector g_CamPosition;

sampler DefaultSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = wrap;
    AddressV = wrap;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal   : NORMAL;
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
    // Terrian의 픽셀 좌표는 Local이기 때문에 World로 맞춰줘야한다. LightDir이 월드공간임
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord * 30.f;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    
    return Out;
}

/* w나누기연산을 수행한다.-> 이 연산으로 이어질수 있는 이유 -> VS_OUT구조체의 위치 -> SV_ */
/* 뷰포트(윈도우좌표)로 변환한다. */
/* 래스터라이즈 -> 정점 세개로 감싸진 영역의 픽셀 정보를 생성한다 */

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    vector vColor : SV_TARGET0;
};

/* Pixel Shader -> 픽셀의 색을 결정한다. */

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector mtrlDiffuse = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);

    vector shade = saturate(max(dot(normalize(g_LightDir) * -1.f, In.vNormal), 0.f) + (g_LightAmbient * g_MtrlAmbiment));

    vector lookDir = In.vWorldPos - g_CamPosition;
    vector refelctDir = reflect(normalize(g_LightDir), In.vNormal);

    float specular = pow(max(dot(normalize(lookDir) * -1.f, normalize(refelctDir)), 0.f), 50.f);
    vector specularColor = g_LightSpecular * g_MtrlSpecular * specular;;
    
    Out.vColor = g_LightDiffuse * mtrlDiffuse * shade + specularColor;
    
    return Out;
}

BlendState OpaqueBlend
{
    BlendEnable[0] = False;
};

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetBlendState(OpaqueBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}

