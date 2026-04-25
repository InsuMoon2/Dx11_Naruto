#include "pch.h"
#include "RenderTarget.h"
#include <wincodec.h>

#include "Shader.h"
#include "VIBuffer_Rect.h"
#pragma comment(lib, "windowscodecs.lib")

RenderTarget::RenderTarget()
{
}

RenderTarget::~RenderTarget()
{
}

HRESULT RenderTarget::Initialize(ComPtr<Device> device, uint32 width, uint32 height,
                        DXGI_FORMAT format, bool createDepth)
{
    _device = device;
    _device->GetImmediateContext(_context.GetAddressOf());
    _width = width;
    _height = height;

    // Color Texture
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = _width;
        desc.Height = _height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(_device->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf())))
            return E_FAIL;
    }

    // RenderTargetView
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = format;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(_device->CreateRenderTargetView(_texture.Get(), &desc, _renderTargetView.GetAddressOf())))
            return E_FAIL;
    }

    // ShaderResourceView (ImGui용)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        if (FAILED(_device->CreateShaderResourceView(_texture.Get(), &desc, _shaderResourceView.GetAddressOf())))
            return E_FAIL;
    }

    if (createDepth)
    {
        // Depth Texture
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = _width;
            desc.Height = _height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            if (FAILED(_device->CreateTexture2D(&desc, nullptr, _depthTexture.GetAddressOf())))
                return E_FAIL;
        }

        // DepthStencilView
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            if (FAILED(_device->CreateDepthStencilView(_depthTexture.Get(), &desc, _depthStencilView.GetAddressOf())))
                return E_FAIL;
        }
    }

    // Viewport
    _viewport.TopLeftX = 0.f;
    _viewport.TopLeftY = 0.f;
    _viewport.Width = static_cast<float>(_width);
    _viewport.Height = static_cast<float>(_height);
    _viewport.MinDepth = 0.f;
    _viewport.MaxDepth = 1.f;

    return S_OK;
}

HRESULT RenderTarget::Resize(uint32 width, uint32 height, DXGI_FORMAT format, bool createDepth)
{
    if (_width == width && _height == height)
        return S_OK;

    if (width == 0 || height == 0)
        return S_OK;

    Release();

    return Initialize(_device, width, height, format, createDepth);
}

void RenderTarget::BindAsTarget()
{
    _context->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());
    _context->RSSetViewports(1, &_viewport);
}

void RenderTarget::Clear(const Color& color)
{
    _context->ClearRenderTargetView(_renderTargetView.Get(), (float*)&color);

    if (_depthStencilView)
        _context->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void RenderTarget::UnbindAll()
{
    ID3D11RenderTargetView* nullRTV = nullptr;
    _context->OMSetRenderTargets(1, &nullRTV, nullptr);

    // 백버퍼 복원은 Graphic_Device에서 처리 ㄱㄱ
}

HRESULT RenderTarget::Save_To_File(const wstring& outputPath)
{
    // Staging 텍스처 생성

    D3D11_TEXTURE2D_DESC stagingDesc = {};

    stagingDesc.Width = _width;
    stagingDesc.Height = _height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ComPtr<ID3D11Texture2D> stagingTex;
    CHECK_FAILED(_device->CreateTexture2D(&stagingDesc, nullptr, stagingTex.GetAddressOf()), E_FAIL);

    _context->CopyResource(stagingTex.Get(), _texture.Get());
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    CHECK_FAILED(_context->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped), E_FAIL);

    // WIC PNG 저장
    IWICImagingFactory* wicFactory = nullptr;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));

    IWICStream* stream = nullptr;
    wicFactory->CreateStream(&stream);
    stream->InitializeFromFilename(outputPath.c_str(), GENERIC_WRITE);

    IWICBitmapEncoder* encoder = nullptr;
    wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    encoder->Initialize(stream, WICBitmapEncoderNoCache);

    IWICBitmapFrameEncode* frame = nullptr;
    encoder->CreateNewFrame(&frame, nullptr);
    frame->Initialize(nullptr);
    frame->SetSize(_width, _height);

    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppRGBA;
    frame->SetPixelFormat(&fmt);
    frame->WritePixels(_height, mapped.RowPitch,
        mapped.RowPitch * _height, (BYTE*)mapped.pData);
    frame->Commit();
    encoder->Commit();

    _context->Unmap(stagingTex.Get(), 0);
    frame->Release();  encoder->Release();
    stream->Release(); wicFactory->Release();

    return S_OK;
}

#ifdef _DEBUG
HRESULT RenderTarget::Ready_Debug(float x, float y, float sizeX, float sizeY)
{
    _debugWorldMatrix = Matrix::Identity;
    _debugWorldMatrix._11 = sizeX;
    _debugWorldMatrix._22 = sizeY;
    _debugWorldMatrix._41 = x;
    _debugWorldMatrix._42 = y;

    return S_OK;
}

HRESULT RenderTarget::Render(Shared<VIBuffer_Rect> viBuffer, Shared<Shader> shader)
{
    CHECK_NULL(viBuffer, E_FAIL);
    CHECK_NULL(shader, E_FAIL);

    shader->Bind_Matrix("g_WorldMatrix", &_debugWorldMatrix);
    shader->Bind_SRV("g_Texture", _shaderResourceView.Get());

    CHECK_FAILED(viBuffer->Bind_Resources(), E_FAIL);
    shader->Begin_Pass(0);
    CHECK_FAILED(viBuffer->Render(), E_FAIL);

    return S_OK;
}

HRESULT RenderTarget::Log_DebugFloatStats(const wstring& targetTag) const
{
    CHECK_NULL(_texture, E_FAIL);

    D3D11_TEXTURE2D_DESC sourceDesc = {};
    _texture->GetDesc(&sourceDesc);

    const bool isR32Float = (sourceDesc.Format == DXGI_FORMAT_R32_FLOAT);
    const bool isRGBA32Float = (sourceDesc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT);
    if (!isR32Float && !isRGBA32Float)
    {
        LOG_INFO("[RTStats] {} skipped. unsupported format={}", Utils::ToString(targetTag), static_cast<uint32>(sourceDesc.Format));
        return S_FALSE;
    }

    D3D11_TEXTURE2D_DESC stagingDesc = sourceDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<Texture2D> stagingTexture;
    CHECK_FAILED(_device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf()), E_FAIL);

    _context->CopyResource(stagingTexture.Get(), _texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    CHECK_FAILED(_context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped), E_FAIL);

    float minValue = FLT_MAX; // RT red 채널의 최소값이다.
    float maxValue = -FLT_MAX; // RT red 채널의 최대값이다.
    float minGreenValue = FLT_MAX; // RT green 채널의 최소값이다.
    float maxGreenValue = -FLT_MAX; // RT green 채널의 최대값이다.
    uint64 belowClearCount = 0; // clear 값 1보다 작은 red 픽셀 개수다.
    uint64 nonClearCount = 0; // RGBA 중 하나라도 clear 값과 달라진 픽셀 개수다.
    uint64 validCount = 0; // 통계를 낸 전체 픽셀 개수다.

    const uint32 componentCount = isRGBA32Float ? 4 : 1;
    for (uint32 y = 0; y < sourceDesc.Height; ++y)
    {
        const auto row = reinterpret_cast<const float*>(
            reinterpret_cast<const uint8*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * y);

        for (uint32 x = 0; x < sourceDesc.Width; ++x)
        {
            const float redValue = row[x * componentCount];
            minValue = min(minValue, redValue);
            maxValue = max(maxValue, redValue);

            if (redValue < 0.999f)
                ++belowClearCount;

            if (isRGBA32Float)
            {
                const float greenValue = row[x * componentCount + 1];
                const float blueValue = row[x * componentCount + 2];
                const float alphaValue = row[x * componentCount + 3];

                minGreenValue = min(minGreenValue, greenValue);
                maxGreenValue = max(maxGreenValue, greenValue);

                if (fabsf(redValue - 1.f) > 0.0005f ||
                    fabsf(greenValue - 1.f) > 0.0005f ||
                    fabsf(blueValue - 1.f) > 0.0005f ||
                    fabsf(alphaValue - 1.f) > 0.0005f)
                {
                    ++nonClearCount;
                }
            }

            ++validCount;
        }
    }

    _context->Unmap(stagingTexture.Get(), 0);

    LOG_INFO("[RTStats] {} format={} size={}x{} minR={:.6f} maxR={:.6f} minG={:.6f} maxG={:.6f} belowClear={} nonClear={} / {}",
        Utils::ToString(targetTag),
        static_cast<uint32>(sourceDesc.Format),
        sourceDesc.Width,
        sourceDesc.Height,
        minValue,
        maxValue,
        (isRGBA32Float ? minGreenValue : minValue),
        (isRGBA32Float ? maxGreenValue : maxValue),
        belowClearCount,
        nonClearCount,
        validCount);

    return S_OK;
}
#endif

void RenderTarget::Release()
{
    _texture.Reset();
    _renderTargetView.Reset();
    _shaderResourceView.Reset();
    _depthTexture.Reset();
    _depthStencilView.Reset();
}

Shared<RenderTarget> RenderTarget::Create(ComPtr<Device> device, uint32 width, uint32 height, DXGI_FORMAT format,
    bool createDepth)
{
    auto instance = make_shared<RenderTarget>();
    if (FAILED(instance->Initialize(device, width, height, format, createDepth)))
    {
        MSG_BOX("Failed to Create : RenderTarget");
        return nullptr;
    }
    return instance;
}

void RenderTarget::Free()
{
    Base::Free();

    Release(); // 스마트 포인터들 순서대로 초기화
}
