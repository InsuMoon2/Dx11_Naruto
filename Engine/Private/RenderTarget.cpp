#include "pch.h"
#include "RenderTarget.h"

RenderTarget::RenderTarget()
{
}

RenderTarget::~RenderTarget()
{
}

HRESULT RenderTarget::Initialize(ComPtr<Device> device, uint32 width, uint32 height)
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
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(_device->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf())))
            return E_FAIL;
    }

    // RenderTargetView
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(_device->CreateRenderTargetView(_texture.Get(), &desc, _renderTargetView.GetAddressOf())))
            return E_FAIL;
    }

    // ShaderResourceView (ImGui용)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        if (FAILED(_device->CreateShaderResourceView(_texture.Get(), &desc, _shaderResourceView.GetAddressOf())))
            return E_FAIL;
    }

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

    // Viewport
    _viewport.TopLeftX = 0.f;
    _viewport.TopLeftY = 0.f;
    _viewport.Width = static_cast<float>(_width);
    _viewport.Height = static_cast<float>(_height);
    _viewport.MinDepth = 0.f;
    _viewport.MaxDepth = 1.f;

    return S_OK;
}

HRESULT RenderTarget::Resize(uint32 width, uint32 height)
{
    if (_width == width && _height == height)
        return S_OK;

    if (width == 0 || height == 0)
        return S_OK;

    Release();

    return Initialize(_device, width, height);
}

void RenderTarget::BindAsTarget()
{
    _context->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());
    _context->RSSetViewports(1, &_viewport);
}

void RenderTarget::Clear(const Color& color)
{
    _context->ClearRenderTargetView(_renderTargetView.Get(), (float*)&color);
    _context->ClearDepthStencilView(_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void RenderTarget::UnbindAll()
{
    ID3D11RenderTargetView* nullRTV = nullptr;
    _context->OMSetRenderTargets(1, &nullRTV, nullptr);

    // 백버퍼 복원은 Graphic_Device에서 처리 ㄱㄱ
}

void RenderTarget::Release()
{
    _texture.Reset();
    _renderTargetView.Reset();
    _shaderResourceView.Reset();
    _depthTexture.Reset();
    _depthStencilView.Reset();
}

shared_ptr<RenderTarget> RenderTarget::Create(ComPtr<Device> device, uint32 width, uint32 height)
{
    auto instance = make_shared<RenderTarget>();
    if (FAILED(instance->Initialize(device, width, height)))
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
