#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL Graphic_Device : public Base
{
public:
    explicit Graphic_Device();
    virtual ~Graphic_Device();

public:
    HRESULT Initialize(HWND hWnd, EWinMode eWinMode,
        uint32 winSizeX, uint32 winSizeY,
        ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);

    HRESULT Clear_BackBufferView(const Color& clearColor);
    HRESULT Clear_DepthStencil_View();

    /* 후면 버퍼를 전면버퍼로 교체한다.(백버퍼를 화면에 직접 보여준다.) */
    HRESULT Present();

public:
    void                        BindBackBuffer();
    ComPtr<Device>              Get_Device()         { return _device; }
    ComPtr<DeviceContext>       Get_Context()  { return _context; }

    float Get_ViewportWidth()   { return _width; }
    float Get_ViewportHeight()  { return _height; }

    float Get_WindowWidth()     { return _originalWidth; }
    float Get_WindowHeight()    { return _originalHeight; }

    ComPtr<SwapChain> Get_SwapChain() { return _swapChain; }

    HRESULT Resize(uint32 width, uint32 height);

    void Clear_DepthOnly();

private:
    HRESULT Ready_SwapChain(HWND hWnd, EWinMode eWinMode, uint32 winSizeX, uint32 winSizeY);
    HRESULT Ready_BackBuffer_RenderTargetView();
    HRESULT Ready_DepthStencilView(uint32 winSizeX, uint32 winSizeY);

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;
    ComPtr<SwapChain>           _swapChain;
    ComPtr<RenderTargetView>    _renderTarget;
    ComPtr<DepthStencil>        _depthStencil;

    float                       _width  = {};
    float                       _height = {};

    float                       _originalWidth = {};
    float                       _originalHeight = {};

public:
    static unique_ptr<Graphic_Device> Create(HWND hWnd, EWinMode eWinMode,
        uint32 winSizeX, uint32 winSizeY,
        ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);

    virtual void Free() override;

};

NS_END
