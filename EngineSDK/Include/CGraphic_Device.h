#pragma once

#include "CBase.h"

BEGIN(Engine)

class ENGINE_DLL CGraphic_Device : public CBase
{
public:
    explicit CGraphic_Device();
    virtual ~CGraphic_Device();

public:
    HRESULT Initialize(HWND hWnd, WINMODE eWinMode,
        uint32 winSizeX, uint32 winSizeY,
        ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);

    HRESULT Clear_BackBufferView(const color& clearColor);
    HRESULT Clear_DepthStencil_View();

    /* 후면 버퍼를 전면버퍼로 교체한다.(백버퍼를 화면에 직접 보여준다.) */
    HRESULT Present();

private:
    HRESULT Ready_SwapChain(HWND hWnd, WINMODE eWinMode, uint32 winSizeX, uint32 winSizeY);
    HRESULT Ready_BackBuffer_RenderTargetView();
    HRESULT Ready_DepthStencilView(uint32 winSizeX, uint32 winSizeY);

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;
    ComPtr<SwapChain>       _swapChain;
    ComPtr<RenderTarget>    _renderTarget;
    ComPtr<DepthStencil>    _depthStencil;

public:
    static unique_ptr<CGraphic_Device> Create(HWND hWnd, WINMODE eWinMode,
        uint32 winSizeX, uint32 winSizeY,
        ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);

    virtual void Free() override;

};

END
