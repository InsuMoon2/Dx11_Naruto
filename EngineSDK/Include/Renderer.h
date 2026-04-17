#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;
class Target_Manager;
class Shader;
class VIBuffer_Rect;
class Component;

class Renderer : public Base
{
public:
    Renderer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Renderer();

public:
    HRESULT Initialize();
    void    Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject);
    void    Draw(bool renderDebugPrimitives = true, bool renderColliders = true, bool renderRTDebug = false);
    HRESULT Draw_Preview();

    void    Backup_RenderGroup();
    void    Restore_RenderGroup();

    void    Resize_DeferredViewport(uint32 width, uint32 height);

#ifdef _DEBUG
    void    Add_DebugRenderGroup(Shared<Component> debugComponent);
#endif

private:
    void    Render_BackgroundUI();
    void    Render_Priority();
    void    Render_NonBlend();
    void    Render_Blend();
    void    Render_UI();

    void    Render_Lights();
    void    Render_NonLight(); // 조명 ㄴㄴ
    void    Render_Combined(); // Diffuse * Shade 최종 합성 -> 백버퍼 출력

    void    Apply_Default3DState();
    void    Apply_UIState();

#ifdef _DEBUG
    void    Render_Debug();
#endif

    HRESULT    Ready_RenderTarget();

public:
    uint32  Get_DrawCallCount() const { return _drawCallCount; }
    void    Reset_DrawCallCount() { _drawCallCount = 0; }

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    uint32                  _drawCallCount = 0;

    ComPtr<ID3D11BlendState>        _uiBlendState;
    ComPtr<ID3D11DepthStencilState> _defaultDepthState;
    ComPtr<ID3D11DepthStencilState> _uiDepthDisabledState;

    list<shared_ptr<GameObject>> _renderObjects[ETOI(ERenderGroup::END)];
    list<shared_ptr<GameObject>> _backupRenderObjects[ETOI(ERenderGroup::END)];

    Shared<Shader>          _deferredShader;
    Shared<VIBuffer_Rect>   _viBuffer;

    Matrix _worldMatrix;
    Matrix _viewMatrix;
    Matrix _projMatrix;

#ifdef _DEBUG
    list<shared_ptr<Component>> _debugComponents;
#endif

public:
    static unique_ptr<Renderer> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;
};

NS_END
