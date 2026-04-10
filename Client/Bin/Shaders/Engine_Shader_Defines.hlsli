// ---------------------------------------------------------
// 엔진 전역 셰이더 인클루드 (Engine_Shader_Defines.hlsli)
// ---------------------------------------------------------

// --- Transform Matrices ---
float4x4 g_WorldMatrix;
float4x4 g_ViewMatrix;
float4x4 g_ProjMatrix;

// --- Camera ---
vector g_CamPosition;

// --- Global Lighting ---
vector g_LightDir;
vector g_LightDiffuse;
vector g_LightAmbient;
vector g_LightSpecular;

// --- Material Default ---
vector g_MtrlAmbient = vector(0.3f, 0.3f, 0.3f, 1.f);
vector g_MtrlSpecular = vector(1.f, 1.f, 1.f, 1.f);

// --- Textures ---
Texture2D g_DiffuseTexture;
Texture2D g_MaskTexture;

// --- Common Sampler ---
sampler DefaultSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = wrap;
    AddressV = wrap;
};

// ---------------------------------------------------------
// 엔진 전역 렌더링 상태 (Render States)
// ---------------------------------------------------------

// --- Rasterizer States ---
RasterizerState RS_Default
{
    FillMode = Solid;
    CullMode = Back;
};

RasterizerState RS_CullNone
{
    FillMode = Solid;
    CullMode = None;
};

RasterizerState RS_CullFront
{
    FillMode = Solid;
    CullMode = Front;
};

// --- Blend States ---
BlendState BS_Default
{
    BlendEnable[0] = false;
};

BlendState BS_AlphaBlend
{
    BlendEnable[0] = true;

    SrcBlend = Src_Alpha;
    DestBlend = Inv_Src_Alpha;
    BlendOp = Add;
};

BlendState BS_Additive
{
    BlendEnable[0] = true;

    SrcBlend  = Src_Alpha;
    DestBlend = One;
    BlendOp   = Add;
};

// --- Depth Stencil States ---
DepthStencilState DSS_Default
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = less_equal;
};

DepthStencilState DSS_ZTest_NoWrite
{
    DepthEnable = true;
    DepthWriteMask = zero;
    DepthFunc = less_equal;
};

DepthStencilState DSS_None
{
    DepthEnable = false;
    DepthWriteMask = zero;

};
