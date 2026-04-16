#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class RenderTarget;
class Shader;
class VIBuffer_Rect;

class ENGINE_DLL Target_Manager : public Base
{
private:
    struct FTargetDesc
    {
        Shared<RenderTarget>    target;
        DXGI_FORMAT             format;
        Color                   clearColor;
        bool                    hasDepth = false;
    };

public:
    explicit Target_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Target_Manager() = default;

public:
    HRESULT Add_RenderTarget(const wstring& targetTag, uint32 sizeX, uint32 sizeY, DXGI_FORMAT pixelFormat, const Color& clearColor);
    HRESULT Add_MRT(const wstring& mrtTag, const wstring& targetTag);
    HRESULT Bind_ShaderResource(Shared<Shader> shader, const char* constantName, const wstring& targetTag);

    HRESULT Resize_MRTs(uint32 width, uint32 height);

    HRESULT Begin_MRT(const wstring& mrtTag, ComPtr<DepthStencil> shaderDSV = nullptr);
    HRESULT End_MRT();

#ifdef _DEBUG
    HRESULT Ready_Debug(const wstring& targetTag, float x, float y, float sizeX, float sizeY);
    HRESULT Render_Debug(const wstring& mrtTag, Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer);

#endif

private:
    Shared<RenderTarget>        Find_RenderTarget(const wstring& targetTag);
    list<Shared<RenderTarget>>* Find_MRT(const wstring& mrtTag);

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    map<wstring, FTargetDesc>                _renderTargets;
    map<wstring, list<Shared<RenderTarget>>> _MRTs;

    ComPtr<RenderTargetView> _backBuffer;
    ComPtr<DepthStencil>     _originalDSV;

    uint32 _currentWidth = 0;
    uint32 _currentHeight = 0;

public:
    static Unique<Target_Manager> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;

};

NS_END
