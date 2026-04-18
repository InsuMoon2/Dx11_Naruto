#include "pch.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Shader.h"
#include "UIObject.h"
#include "UI_Text.h"
#include "VIBuffer_Rect.h"

Renderer::Renderer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
    
}

Renderer::~Renderer()
{

}

HRESULT Renderer::Initialize()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(_device->CreateBlendState(&blendDesc, _uiBlendState.GetAddressOf())))
        return E_FAIL;

    // --------------------------------------------------
    // 기본 3D depth on state
    // --------------------------------------------------
    D3D11_DEPTH_STENCIL_DESC defaultDepthDesc = {};
    defaultDepthDesc.DepthEnable = TRUE;
    defaultDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    defaultDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    defaultDepthDesc.StencilEnable = FALSE;

    if (FAILED(_device->CreateDepthStencilState(&defaultDepthDesc, _defaultDepthState.GetAddressOf())))
        return E_FAIL;

    // --------------------------------------------------
    // UI depth off state
    // --------------------------------------------------
    D3D11_DEPTH_STENCIL_DESC uiDepthDesc = {};
    uiDepthDesc.DepthEnable = FALSE;
    uiDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    uiDepthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    uiDepthDesc.StencilEnable = FALSE;

    if (FAILED(_device->CreateDepthStencilState(&uiDepthDesc, _uiDepthDisabledState.GetAddressOf())))
        return E_FAIL;

    CHECK_FAILED(Ready_RenderTarget(), E_FAIL);

    return S_OK;
}

void Renderer::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    CHECK_NULL(gameObject);

    _renderObjects[ETOI(renderType)].emplace_back(gameObject);
}

void Renderer::Backup_RenderGroup()
{
    for (int i = 0; i < ETOI(ERenderGroup::END); ++i)
    {
        _backupRenderObjects[i] = _renderObjects[i];
        _renderObjects[i].clear();
    }
}

void Renderer::Restore_RenderGroup()
{
    for (int i = 0; i < ETOI(ERenderGroup::END); ++i)
    {
        _renderObjects[i] = _backupRenderObjects[i];
    }
}

void Renderer::Resize_DeferredViewport(uint32 width, uint32 height)
{
	if (width == 0 || height == 0)
        return;

    _worldMatrix = Matrix::CreateScale(static_cast<float>(width), static_cast<float>(height), 1.f);
    _viewMatrix = Matrix::Identity;
    _projMatrix = XMMatrixOrthographicLH(static_cast<float>(width), static_cast<float>(height), 0.f, 1.f);
}

#ifdef _DEBUG
void Renderer::Add_DebugRenderGroup(Shared<Component> debugComponent)
{
    _debugComponents.push_back(debugComponent);
}
#endif

void Renderer::Draw(bool renderDebugPrimitives, bool renderColliders, bool renderRTDebug)
{
    _drawCallCount = 0;

    Render_BackgroundUI();
    Apply_Default3DState();
    Render_Priority();

    Render_NonBlend();

    Render_Lights();
    Render_Combined();
    Render_NonLight();

    Render_Blend();

    Apply_Default3DState();

    if (renderDebugPrimitives)
        GAME->Render_DebugDepth();

    Apply_UIState();

#ifdef _DEBUG
    if (renderColliders)
        GAME->Render_Colliders();
#endif

    if (renderDebugPrimitives)
        GAME->Render_DebugOverlay();

    Apply_Default3DState();

    Render_UI();

#ifdef _DEBUG
    if (renderRTDebug)
        Render_Debug();
#endif
}

HRESULT Renderer::Draw_Preview()
{
    _drawCallCount = 0;

    Apply_Default3DState();
    Render_Priority();
    
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonBlend)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }
    _renderObjects[ETOI(ERenderGroup::NonBlend)].clear();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonLight)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }
    _renderObjects[ETOI(ERenderGroup::NonLight)].clear();

    Render_Blend();

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].clear();
    _renderObjects[ETOI(ERenderGroup::UI)].clear();
    
    return S_OK;
}

void Renderer::Render_BackgroundUI()
{
    Apply_UIState();

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].sort([](const Shared<GameObject>& src, const Shared<GameObject>& dst) {
        auto uiSrc = dynamic_pointer_cast<UIObject>(src);
        auto uiDst = dynamic_pointer_cast<UIObject>(dst);
        return uiSrc->Get_ZOrder() < uiDst->Get_ZOrder();
        });

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::BackgroundUI)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].clear();

    Apply_Default3DState();
}

void Renderer::Render_Priority()
{
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Priority)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Priority)].clear();
}

void Renderer::Render_NonBlend()
{
    CHECK_FAILED(GAME->Begin_MRT(L"MRT_GameObjects"));

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonBlend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::NonBlend)].clear();

    GAME->End_MRT();
}

void Renderer::Render_Blend()
{
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Blend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Blend)].clear();

    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Renderer::Render_UI()
{
    Apply_UIState();

    _renderObjects[ETOI(ERenderGroup::UI)].sort([](const Shared<GameObject>& src, const Shared<GameObject>& dst)
        {
            auto uiSrc = dynamic_pointer_cast<UIObject>(src);
            auto uiDst = dynamic_pointer_cast<UIObject>(dst);
    
            if (uiSrc->Get_UILayer() != uiDst->Get_UILayer())
            {
                return uiSrc->Get_UILayer() < uiDst->Get_UILayer();
            }
    
            // 같은 레이어면, ZOrder 기준
            return uiSrc->Get_ZOrder() < uiDst->Get_ZOrder();
    
        });

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::UI)])
    {
        if (!renderObject)
            continue;

        // DWrite는 별도 패스에서 처리해 D3D UI와 같은 백버퍼에 섞어 그리지 않는다.
        if (dynamic_pointer_cast<UI_Text>(renderObject))
            continue;

        renderObject->Render();

        _drawCallCount++;
    }

    ID3D11RenderTargetView* nullRTV = nullptr;
    _context->OMSetRenderTargets(1, &nullRTV, nullptr);

    GAME->Begin_UIText();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::UI)])
    {
        auto uiText = dynamic_pointer_cast<UI_Text>(renderObject);
        if (!uiText)
            continue;

        uiText->Render();
        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::UI)].clear();

    GAME->End_UIText();
    Apply_Default3DState();
}

void Renderer::Render_Lights()
{
    if (FAILED(GAME->Begin_MRT(L"MRT_LightAcc")))
        return;

    bool canRender = true;

    if (FAILED(_deferredShader->Bind_Matrix("g_WorldMatrix", &_worldMatrix)))
        canRender = false;

    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix", &_viewMatrix)))
        canRender = false;

    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix", &_projMatrix)))
        canRender = false;

    if (canRender &&
        FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_NormalTexture", L"Target_Normal")))
    {
        canRender = false;
    }

    if (canRender)
    {
        _viBuffer->Bind_Resources();

        if (FAILED(GAME->Render_Lights(_deferredShader, _viBuffer)))
            canRender = false;
    }

    GAME->End_MRT();
}


void Renderer::Render_NonLight()
{
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonLight)])
    {
        if (renderObject)
            renderObject->Render();
        _drawCallCount++;
    }
    _renderObjects[ETOI(ERenderGroup::NonLight)].clear();
}

void Renderer::Render_Combined()
{
    if (FAILED(_deferredShader->Bind_Matrix("g_WorldMatrix", &_worldMatrix))) return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix",  &_viewMatrix)))  return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix",  &_projMatrix)))  return;

    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_DiffuseTexture", L"Target_Diffuse"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_NormalTexture", L"Target_Normal"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_ShadeTexture",   L"Target_Shade")))   return;

    // 포스트 프로세스 외곽선 처리
    const float outlineInvViewportSize[2] =
    {
        1.f / max(1.f, static_cast<float>(GAME->Get_ViewportWidth())),
        1.f / max(1.f, static_cast<float>(GAME->Get_ViewportHeight()))
    };

    const float outlineNormalThreshold = 0.32f;
    const float outlineStrength = 0.45f;
    const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);

    if (FAILED(_deferredShader->Bind_RawValue("g_OutlineInvViewportSize", outlineInvViewportSize, sizeof(outlineInvViewportSize))))
        return;

    if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineNormalThreshold", &outlineNormalThreshold, sizeof(float))))
        return;

    if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineStrength", &outlineStrength, sizeof(float))))
        return;

    if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineColor", &outlineColor, sizeof(Vec4))))
        return;

    _deferredShader->Begin_Pass(ETOI(EDeferred::Combined));

    _viBuffer->Bind_Resources();
    _viBuffer->Render();
}

void Renderer::Apply_Default3DState()
{
    _context->OMSetDepthStencilState(_defaultDepthState.Get(), 0);
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Renderer::Apply_UIState()
{
    _context->OMSetDepthStencilState(_uiDepthDisabledState.Get(), 0);
    _context->OMSetBlendState(_uiBlendState.Get(), nullptr, 0xffffffff);

}

void Renderer::Render_Debug()
{
    for (auto& debugComponent : _debugComponents)
        debugComponent->Render_Debug();

    _debugComponents.clear();

    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix", &_viewMatrix))) return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix", &_projMatrix))) return;

    if (FAILED(GAME->Render_RT_Debug(_viBuffer, _deferredShader, L"MRT_GameObjects"))) return;
    if (FAILED(GAME->Render_RT_Debug(_viBuffer, _deferredShader, L"MRT_LightAcc")))    return;
}

HRESULT Renderer::Ready_RenderTarget()
{
    uint32 numViewports = 1;

    const uint32 width = static_cast<uint32>(GAME->Get_ViewportWidth());
    const uint32 height = static_cast<uint32>(GAME->Get_ViewportHeight());

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Diffuse",
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM, Color(0,0,0,0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Normal",
        width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(0,0,0,0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Shade",
        width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(0,0,0,0)), E_FAIL);


    CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Diffuse"), E_FAIL); // SV_TARGET0
    CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Normal"),  E_FAIL); // SV_TARGET1
    CHECK_FAILED(GAME->Add_MRT(L"MRT_LightAcc",    L"Target_Shade"),   E_FAIL); // SV_TARGET0

    // Deferred Shader, 풀스크린 Rect 생성
    _viBuffer = VIBuffer_Rect::Create(_device, _context);
    CHECK_NULL(_viBuffer, E_FAIL);

    _deferredShader = Shader::Create(_device, _context,
        L"../../Client/Bin/Shaders/Shader_Deferred.hlsl", VTXTEX::Elements, VTXTEX::numElements);

    if (!_deferredShader) return E_FAIL;

    // 직교투영 행렬 세팅
    _worldMatrix = Matrix::CreateScale(width, height, 1.f);
    _viewMatrix = Matrix::Identity;
    _projMatrix = XMMatrixOrthographicLH(width, height, 0.f, 1.f);

#ifdef _DEBUG
    CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Diffuse", 150.f, 150.f, 300.f, 300.f), E_FAIL);
    CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Normal",  150.f, 450.f, 300.f, 300.f), E_FAIL);
    CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Shade",   450.f, 150.f, 300.f, 300.f), E_FAIL);
#endif

    return S_OK;
}

unique_ptr<Renderer> Renderer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_unique<Renderer>(device, context);

    if (FAILED(instance->Initialize()))
    {
        return nullptr; 
    }

    return instance;
}

void Renderer::Free()
{
    Base::Free();

    for (auto& renderObjects : _renderObjects)
    {
        renderObjects.clear();
    }
}
