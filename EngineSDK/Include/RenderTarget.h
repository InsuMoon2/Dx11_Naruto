#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class VIBuffer_Rect;
class Shader;

class ENGINE_DLL RenderTarget : public Base
{
public:
    explicit RenderTarget();
    virtual ~RenderTarget();

public:
    HRESULT Initialize(ComPtr<Device> device, uint32 width, uint32 height,
                        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool createDepth = true);

    HRESULT Resize(uint32 width, uint32 height,
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,bool createDepth = true);


    void    BindAsTarget();
    
    void    Clear(const Color& color = Color(0.f, 0.f, 0.f, 0.f));
    void    UnbindAll();

public:
    ID3D11ShaderResourceView* Get_SRV() const { return _shaderResourceView.Get(); }
    ComPtr<ID3D11ShaderResourceView> Get_SRV_ComPtr() const { return _shaderResourceView; }

    ID3D11RenderTargetView* Get_RTV() const { return _renderTargetView.Get(); }

    uint32 GetWidth() const { return _width; }
    uint32 GetHeight() const { return _height; }

    HRESULT Save_To_File(const wstring& outputPath);

    ComPtr<Texture2D> Get_Texture2D() const { return _texture; }

    uint32 Get_Width() const { return _width; }
    uint32 Get_Height() const { return _height; }

    #ifdef _DEBUG
public:
    HRESULT Ready_Debug(float x, float y, float sizeX, float sizeY);
    HRESULT Render(Shared<VIBuffer_Rect> viBuffer, Shared<Shader> shader);
private:
    Matrix _debugWorldMatrix = Matrix::Identity;
#endif


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
     static Shared<RenderTarget> Create(ComPtr<Device> device, uint32 width, uint32 height, 
                                           DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, 
                                           bool createDepth = true);

    virtual void Free() override;

};

NS_END
