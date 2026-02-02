#include "pch.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"

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
