#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL RenderTarget : public Base
{
public:
    explicit RenderTarget();
    virtual ~RenderTarget();

public:
    HRESULT Initialize(ComPtr<Device> device, uint32 width, uint32 height);
    HRESULT Resize(uint32 width, uint32 height);

    void    BindAsTarget();
    void    Clear(const Color& color = Color(0.2f, 0.2f, 0.2f, 1.f));
    void    UnbindAll();

public:
    ImTextureID GetSRV() const { return (ImTextureID)_shaderResourceView.Get(); }

    uint32 GetWidth() const { return _width; }
    uint32 GetHeight() const { return _height; }

private:
    void Release();

private:
    ComPtr<Device>                   _device;
    ComPtr<DeviceContext>            _context;

    uint32                           _width  = {};
    uint32                           _height = {};

    /* Color Buffer */
    ComPtr<Texture2D>                _texture;
    ComPtr<ID3D11RenderTargetView>   _renderTargetView;
    ComPtr<ID3D11ShaderResourceView> _shaderResourceView;

    /* Depth Buffer */
    ComPtr<Texture2D>               _depthTexture;
    ComPtr<DepthStencil>            _depthStencilView;

    /* Viewport */
    D3D11_VIEWPORT                  _viewport = {};

public:
    static shared_ptr<RenderTarget> Create(ComPtr<Device> device, uint32 width, uint32 height);
    virtual void Free() override;

};

NS_END
