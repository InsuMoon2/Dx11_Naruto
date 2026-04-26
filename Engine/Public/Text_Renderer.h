#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Text_Renderer : public Base
{
public:
    Text_Renderer(ComPtr<Device> device, ComPtr<SwapChain> swapChain);
    virtual ~Text_Renderer() = default;

public:
    HRESULT Initialize();

    HRESULT Begin_UIText();
    HRESULT Draw_Text(const wstring& text, const RECT& rect, const FTextStyle& style);
    HRESULT End_UIText();

    void    On_BeforeResize();
    HRESULT On_AfterResize();

    HRESULT Set_TargetTexture(ComPtr<Texture2D> texture);
    HRESULT Reset_TargetToSwapChain();

private:
    HRESULT Create_TargetBitmap_FromTexture(ComPtr<Texture2D> texture);

private:
    struct FCachedBrush
    {
        Color color;
        ComPtr<ID2D1SolidColorBrush> brush;
    };

    struct FCachedFormat
    {
        wstring fontFamily;

        float fontSize = 0.f;

        ETextHAlign hAlign = ETextHAlign::Left;
        ETextVAlign vAlign = ETextVAlign::Top;

        bool wordWrap = false;
        ComPtr<IDWriteTextFormat> format;
    };

private:
    HRESULT                 Create_DeviceResources();
    HRESULT                 Create_TargetBitmap();

    // 로컬 리소스 폰트 파일을 DirectWrite 전용 컬렉션으로 등록할 때 호출한다.
    HRESULT                 Load_DefaultUIFont();

    ID2D1SolidColorBrush*   Find_Or_CreateBrush(const Color& color);
    IDWriteTextFormat*      Find_Or_CreateFormat(const FTextStyle& style);

private:
    ComPtr<Device>              _device;
    ComPtr<SwapChain>           _swapChain;

    ComPtr<ID2D1Factory1>       _d2dFactory;
    ComPtr<ID2D1Device>         _d2dDevice;
    ComPtr<ID2D1DeviceContext>  _d2dContext;
    ComPtr<IDWriteFactory>      _writeFactory;
    // OpenSans SemiBold를 시스템 설치 없이 사용할 수 있게 들고 있는 전용 폰트 컬렉션이다.
    ComPtr<IDWriteFontCollection1> _defaultUIFontCollection;
    ComPtr<ID2D1Bitmap1>        _targetBitmap;

    vector<FCachedBrush>        _brushCache;
    vector<FCachedFormat>       _formatCache;

    bool _frameBegun = false;

public:
    static Unique<Text_Renderer> Create(ComPtr<Device> device, ComPtr<SwapChain> swapChain);
    virtual void Free() override;
        
};

NS_END
