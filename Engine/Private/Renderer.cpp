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
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Blend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Blend)].clear();
}

void Renderer::Render_UI()
{
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
