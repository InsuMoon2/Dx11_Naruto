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
    void    Clear(const Color& color = Color(0.53f, 0.81f, 0.92f, 1.f));
    void    UnbindAll();

public:
    ID3D11ShaderResourceView* Get_SRV() const { return _shaderResourceView.Get(); }
    ComPtr<ID3D11ShaderResourceView> Get_SRV_ComPtr() const { return _shaderResourceView; }

    uint32 GetWidth() const { return _width; }
    uint32 GetHeight() const { return _height; }

    HRESULT Save_To_File(const wstring& outputPath);

    ComPtr<Texture2D> Get_Texture2D() const { return _texture; }

    uint32 Get_Width() const { return _width; }
    uint32 Get_Height() const { return _height; }

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

    Color                           _clearColor = Color(0.53f, 0.81f, 0.92f, 1.f);

public:
    static shared_ptr<RenderTarget> Create(ComPtr<Device> device, uint32 width, uint32 height);
    virtual void Free() override;

};

NS_END
