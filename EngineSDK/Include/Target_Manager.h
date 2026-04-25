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
        bool                    resizeWithViewport = true; // backbuffer resize 때 같이 늘릴지
    };

public:
    explicit Target_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Target_Manager() = default;

public:
    HRESULT Add_RenderTarget(const wstring& targetTag, uint32 sizeX, uint32 sizeY,
        DXGI_FORMAT pixelFormat, const Color& clearColor, bool resizeWithViewport = true);

    HRESULT Add_MRT(const wstring& mrtTag, const wstring& targetTag);
    HRESULT Bind_ShaderResource(Shared<Shader> shader, const char* constantName, const wstring& targetTag);

    HRESULT Copy_CurrentRenderTargetTo(const wstring& targetTag);

    HRESULT Resize_MRTs(uint32 width, uint32 height);

    HRESULT Begin_MRT(const wstring& mrtTag, ComPtr<DepthStencil> shaderDSV = nullptr);
    HRESULT End_MRT();

#ifdef _DEBUG
    HRESULT Ready_Debug(const wstring& targetTag, float x, float y, float sizeX, float sizeY);
    HRESULT Render_Debug(const wstring& mrtTag, Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer);

    // Debug shadow/RT 진단용: 지정 RT의 float 픽셀 통계를 로그로 찍어 실제 렌더 여부를 확인한다.
    HRESULT Log_DebugFloatStats(const wstring& targetTag);

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

    // MRT 진입 전 viewport 개수다. Shadow MRT처럼 별도 viewport를 쓰는 패스 종료 후 원래 렌더 viewport를 복원하기 위해 저장한다.
    uint32 _originalViewportCount = 0;
    // MRT 진입 전 viewport 목록이다. Editor Scene/Game RT와 백버퍼 viewport를 모두 안전하게 되돌리기 위해 보관한다.
    D3D11_VIEWPORT _originalViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

    uint32 _currentWidth = 0;
    uint32 _currentHeight = 0;

public:
    static Unique<Target_Manager> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;

};

NS_END
