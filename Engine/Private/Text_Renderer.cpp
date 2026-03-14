#include "pch.h"
#include "Text_Renderer.h"

Text_Renderer::Text_Renderer(ComPtr<Device> device, ComPtr<SwapChain> swapChain)
    : _device(device), _swapChain(swapChain)
{
}

HRESULT Text_Renderer::Initialize()
{
    CHECK_FAILED(Create_DeviceResources(), E_FAIL);
    CHECK_FAILED(Create_TargetBitmap() , E_FAIL);

    return S_OK;
}

HRESULT Text_Renderer::Begin_UIText()
{
    if (_frameBegun)
        return S_OK;

    CHECK_NULL(_d2dContext, E_FAIL);

    if (_targetBitmap == nullptr)
    {
        CHECK_FAILED(Create_TargetBitmap(), E_FAIL);
    }

    _d2dContext->BeginDraw();
    _frameBegun = true;

    return S_OK;
}

HRESULT Text_Renderer::Draw_Text(const wstring& text, const RECT& rect, const FTextStyle& style)
{
    if (!_frameBegun || text.empty())
        return S_OK;

    CHECK_NULL(_d2dContext, E_FAIL);

    auto* format = Find_Or_CreateFormat(style);
    auto* brush  = Find_Or_CreateBrush(style.color);

    D2D1_RECT_F rectF = D2D1::RectF(
        static_cast<float>(rect.left),
        static_cast<float>(rect.top),
        static_cast<float>(rect.right),
        static_cast<float>(rect.bottom));

    CHECK_NULL(format, E_FAIL);
    CHECK_NULL(brush, E_FAIL);

    _d2dContext->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        rectF,
        brush);

    return S_OK;
}

HRESULT Text_Renderer::End_UIText()
{
    if (!_frameBegun)
        return S_OK;

    if (_d2dContext == nullptr)
    {
        _frameBegun = false;
        return E_FAIL;
    }

    _frameBegun = false;

    HRESULT hr = _d2dContext->EndDraw();

    if (FAILED(hr))
    {
        LOG_ERROR("Text_Renderer::End_UIText - EndDraw failed. hr={}", static_cast<unsigned int>(hr));
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        _targetBitmap.Reset();
        _d2dContext->SetTarget(nullptr);
        return S_OK;
    }

    CHECK_FAILED(hr, E_FAIL);

    return S_OK;
}

void Text_Renderer::On_BeforeResize()
{
    if (_d2dContext)
    {
        if (_frameBegun)
        {
            _d2dContext->EndDraw();
            _frameBegun = false;
        }

        _d2dContext->SetTarget(nullptr);
    }

    _targetBitmap.Reset();
}

HRESULT Text_Renderer::On_AfterResize()
{
    CHECK_NULL(_swapChain, E_FAIL);
    CHECK_NULL(_d2dContext, E_FAIL);

    CHECK_FAILED(Create_TargetBitmap(), E_FAIL);

    return S_OK;
}

HRESULT Text_Renderer::Set_TargetTexture(ComPtr<Texture2D> texture)
{
    CHECK_NULL(_d2dContext, E_FAIL);
    CHECK_NULL(texture, E_FAIL);

    if (_frameBegun)
        return E_FAIL;

    _d2dContext->SetTarget(nullptr);
    _targetBitmap.Reset();

    return Create_TargetBitmap_FromTexture(texture);
}

HRESULT Text_Renderer::Reset_TargetToSwapChain()
{
    CHECK_NULL(_d2dContext, E_FAIL);
    CHECK_NULL(_swapChain, E_FAIL);

    if (_frameBegun)
        return E_FAIL;

    _d2dContext->SetTarget(nullptr);
    _targetBitmap.Reset();

    return Create_TargetBitmap();
}

HRESULT Text_Renderer::Create_TargetBitmap_FromTexture(ComPtr<Texture2D> texture)
{
    CHECK_NULL(_d2dContext, E_FAIL);
    CHECK_NULL(texture, E_FAIL);

    HRESULT hr = S_OK;

    ComPtr<IDXGISurface> dxgiSurface;
    hr = texture.As(&dxgiSurface);
    CHECK_FAILED(hr, E_FAIL);

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    hr = _d2dContext->CreateBitmapFromDxgiSurface(
        dxgiSurface.Get(),
        &props,
        _targetBitmap.GetAddressOf());
    CHECK_FAILED(hr, E_FAIL);

    _d2dContext->SetTarget(_targetBitmap.Get());
    return S_OK;
}

HRESULT Text_Renderer::Create_DeviceResources()
{
    HRESULT hr = S_OK;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(_writeFactory.GetAddressOf()));
    CHECK_FAILED(hr, E_FAIL);

    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        _d2dFactory.GetAddressOf());
    CHECK_FAILED(hr, E_FAIL);

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = _device.As(&dxgiDevice);
    CHECK_FAILED(hr, E_FAIL);

    hr = _d2dFactory->CreateDevice(dxgiDevice.Get(), _d2dDevice.GetAddressOf());
    CHECK_FAILED(hr, E_FAIL);

    hr = _d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        _d2dContext.GetAddressOf());
    CHECK_FAILED(hr, E_FAIL);

    return S_OK;
    
}

HRESULT Text_Renderer::Create_TargetBitmap()
{
    CHECK_NULL(_swapChain, E_FAIL);
    CHECK_NULL(_d2dContext, E_FAIL);

    HRESULT hr = S_OK;

    ComPtr<IDXGISurface> dxgiSurface;
    hr = _swapChain->GetBuffer(0, IID_PPV_ARGS(dxgiSurface.GetAddressOf()));
    CHECK_FAILED(hr, E_FAIL);

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    hr = _d2dContext->CreateBitmapFromDxgiSurface(
        dxgiSurface.Get(),
        &props,
        _targetBitmap.GetAddressOf());

    CHECK_FAILED(hr, E_FAIL);

    _d2dContext->SetTarget(_targetBitmap.Get());

    return S_OK;
}

ID2D1SolidColorBrush* Text_Renderer::Find_Or_CreateBrush(const Color& color)
{
    for (auto& cached : _brushCache)
    {
        if (cached.color.x == color.x &&
            cached.color.y == color.y &&
            cached.color.z == color.z &&
            cached.color.w == color.w)
        {
            return cached.brush.Get();
        }
    }

    if (_d2dContext == nullptr)
        return nullptr;

    HRESULT hr = S_OK;

    FCachedBrush entry;
    entry.color = color;

    hr = _d2dContext->CreateSolidColorBrush(
        D2D1::ColorF(color.x, color.y, color.z, color.w),
        entry.brush.GetAddressOf());

    if (FAILED(hr))
        return nullptr;

    _brushCache.push_back(entry);
    return _brushCache.back().brush.Get();
}

IDWriteTextFormat* Text_Renderer::Find_Or_CreateFormat(const FTextStyle& style)
{
    for (auto& cached : _formatCache)
    {
        if (cached.fontFamily == style.fontFamily &&
            cached.fontSize == style.fontSize &&
            cached.hAlign == style.hAlign &&
            cached.vAlign == style.vAlign &&
            cached.wordWrap == style.wordWrap)
        {
            return cached.format.Get();
        }
    }

    if (_writeFactory == nullptr)
        return nullptr;

    HRESULT hr = S_OK;

    FCachedFormat entry;
    entry.fontFamily = style.fontFamily;
    entry.fontSize = style.fontSize;
    entry.hAlign = style.hAlign;
    entry.vAlign = style.vAlign;
    entry.wordWrap = style.wordWrap;

    hr = _writeFactory->CreateTextFormat(
        entry.fontFamily.c_str(),
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        entry.fontSize,
        L"ko-KR",
        entry.format.GetAddressOf());

    if (FAILED(hr))
        return nullptr;

    hr = entry.format->SetWordWrapping(
        entry.wordWrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
    if (FAILED(hr))
        return nullptr;

    switch (entry.hAlign)
    {
    case ETextHAlign::Left:
        hr = entry.format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        break;
    case ETextHAlign::Center:
        hr = entry.format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    case ETextHAlign::Right:
        hr = entry.format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        break;
    default:
        hr = S_OK;
        break;
    }

    if (FAILED(hr))
        return nullptr;

    switch (entry.vAlign)
    {
    case ETextVAlign::Top:
        hr = entry.format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        break;
    case ETextVAlign::Middle:
        hr = entry.format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        break;
    case ETextVAlign::Bottom:
        hr = entry.format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        break;
    default:
        hr = S_OK;
        break;
    }

    if (FAILED(hr))
        return nullptr;

    _formatCache.push_back(entry);
    return _formatCache.back().format.Get();
}

Unique<Text_Renderer> Text_Renderer::Create(ComPtr<Device> device, ComPtr<SwapChain> swapChain)
{
    auto instance = make_unique<Text_Renderer>(device, swapChain);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Text_Renderer");

        return nullptr;
    }

    return instance;
}

void Text_Renderer::Free()
{
    Base::Free();
}
