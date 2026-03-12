#include "pch.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "UIObject.h"

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
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
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

    return S_OK;
}

void Renderer::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    CHECK_NULL(gameObject);

    _renderObjects[ETOI(renderType)].emplace_back(gameObject);
}

void Renderer::Draw()
{
    _drawCallCount = 0;

    Apply_Default3DState();

    Render_Priority();

    Render_NonBlend();

    Render_Blend();

    Render_UI();
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
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonBlend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::NonBlend)].clear();
}

void Renderer::Render_Blend()
{
    //_context->OMSetBlendState(_uiBlendState.Get(), nullptr, 0xffffffff);

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Blend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Blend)].clear();

    //_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
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
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::UI)].clear();

    Apply_Default3DState();
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

unique_ptr<Renderer> Renderer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_unique<Renderer>(device, context);

    instance->Initialize();

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
