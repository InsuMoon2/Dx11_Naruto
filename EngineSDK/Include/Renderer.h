#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class Renderer : public Base
{
public:
    Renderer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Renderer();

public:
    HRESULT Initialize();
    void    Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject);
    void    Draw();

private:
    void    Render_Priority();
    void    Render_NonBlend();
    void    Render_Blend();
    void    Render_UI();

    void    Apply_Default3DState();
    void    Apply_UIState();

public:
    int32   Get_DrawCallCount() const { return _drawCallCount; }
    void    Reset_DrawCallCount() { _drawCallCount = 0; }

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    uint32                  _drawCallCount = 0;

    ComPtr<ID3D11BlendState>        _uiBlendState;
    ComPtr<ID3D11DepthStencilState> _defaultDepthState;
    ComPtr<ID3D11DepthStencilState> _uiDepthDisabledState;

    list<shared_ptr<GameObject>> _renderObjects[ETOI(ERenderGroup::END)];

public:
    static unique_ptr<Renderer> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;
};

NS_END
