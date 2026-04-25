#include "pch.h"
#include "Target_Manager.h"
#include "RenderTarget.h"
#include "Shader.h"

Target_Manager::Target_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT Target_Manager::Add_RenderTarget(const wstring& targetTag, uint32 sizeX, uint32 sizeY, DXGI_FORMAT pixelFormat, const Color& clearColor, bool resizeWithViewport)
{
    if (Find_RenderTarget(targetTag))
        return E_FAIL;

    auto target = make_shared<RenderTarget>();

    if (FAILED(target->Initialize(_device, sizeX, sizeY, pixelFormat, false)))
        return E_FAIL;

    target->Clear(clearColor);

    FTargetDesc desc;
    desc.target = target;
    desc.format = pixelFormat;
    desc.clearColor = clearColor;
    desc.hasDepth = false;
    desc.resizeWithViewport = resizeWithViewport;

    _renderTargets.emplace(targetTag, desc);

    if (resizeWithViewport)
    {
        _currentWidth = sizeX;
        _currentHeight = sizeY;
    }

    return S_OK;
}

HRESULT Target_Manager::Add_MRT(const wstring& mrtTag, const wstring& targetTag)
{
    auto renderTarget = Find_RenderTarget(targetTag);
    CHECK_NULL(renderTarget, E_FAIL);

    auto mrtList = Find_MRT(mrtTag);

    if (!mrtList)
    {
        // 신규 그룹이 없으면 만들어서 세팅
        list<Shared<RenderTarget>> newList;
        newList.push_back(renderTarget);

        _MRTs.emplace(mrtTag, newList);
    }
    else
    {
        mrtList->push_back(renderTarget);
    }

    return S_OK;
}

HRESULT Target_Manager::Bind_ShaderResource(Shared<Shader> shader, const char* constantName, const wstring& targetTag)
{
    auto renderTarget = Find_RenderTarget(targetTag);
    CHECK_NULL(renderTarget, E_FAIL);

    return shader->Bind_SRV(constantName, renderTarget->Get_SRV());
}

HRESULT Target_Manager::Copy_CurrentRenderTargetTo(const wstring& targetTag)
{
    // 복사결과를 받을 렌더 타겟
    auto destinationTarget = Find_RenderTarget(targetTag);
    CHECK_NULL(destinationTarget, E_FAIL);

    ComPtr<RenderTargetView> currentRTV; // 현재 화면에 그려지고 있는 RTV
    _context->OMGetRenderTargets(1, currentRTV.GetAddressOf(), nullptr);
    CHECK_NULL(currentRTV, E_FAIL);

    ComPtr<ID3D11Resource> sourceResource; // 현재 RTV가 감싸고 있는 원본 리소스다.
    currentRTV->GetResource(sourceResource.GetAddressOf());
    CHECK_NULL(sourceResource, E_FAIL);

    ComPtr<ID3D11Texture2D> sourceTexture; // CopyResource에 사용할 현재 화면 텍스처다.
    CHECK_FAILED(sourceResource.As(&sourceTexture), E_FAIL);

    ComPtr<ID3D11Texture2D> destinationTexture = destinationTarget->Get_Texture2D(); // SceneColorCopy 텍스처다.
    CHECK_NULL(destinationTexture, E_FAIL);

    if (sourceTexture.Get() == destinationTexture.Get())
        return S_OK;

    D3D11_TEXTURE2D_DESC sourceDesc{}; // 복사 가능 여부 확인용 원본 텍스처 정보다.
    D3D11_TEXTURE2D_DESC destinationDesc{}; // 복사 가능 여부 확인용 대상 텍스처 정보다.
    sourceTexture->GetDesc(&sourceDesc);
    destinationTexture->GetDesc(&destinationDesc);

    if (sourceDesc.Width != destinationDesc.Width ||
        sourceDesc.Height != destinationDesc.Height ||
        sourceDesc.Format != destinationDesc.Format)
    {
        return E_FAIL;
    }

    _context->CopyResource(destinationTexture.Get(), sourceTexture.Get());
    return S_OK;
}

HRESULT Target_Manager::Resize_MRTs(uint32 width, uint32 height)
{
    if (width == 0 || height == 0) 
        return S_OK;

    for (auto& pair : _renderTargets)
    {
        FTargetDesc& desc = pair.second;

        if (!desc.resizeWithViewport)
            continue;

        if (FAILED(desc.target->Resize(width, height, desc.format, desc.hasDepth)))
            return E_FAIL;
    }
    
    _currentWidth = width;
    _currentHeight = height;
    
    return S_OK;
}

HRESULT Target_Manager::Begin_MRT(const wstring& mrtTag, ComPtr<DepthStencil> shaderDSV)
{
    auto mrtList = Find_MRT(mrtTag);
    CHECK_NULL(mrtList, E_FAIL);

    _originalViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    _context->RSGetViewports(&_originalViewportCount, _originalViewports);

    _context->OMGetRenderTargets(1, _backBuffer.ReleaseAndGetAddressOf(), _originalDSV.ReleaseAndGetAddressOf());

    ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    _context->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
    _context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);

    ID3D11RenderTargetView* renderTargets[8] = { nullptr };
    uint32 iNumRenderTargets = 0;
    D3D11_VIEWPORT mrtViewport = {};
    bool hasMRTViewport = false;

    for (auto& renderTarget : *mrtList)
    {
        auto findIter = _renderTargets.begin();

        for (auto & desc : _renderTargets)
        {
            if (desc.second.target == renderTarget)
            {
                renderTarget->Clear(desc.second.clearColor);
                break;
            }
        }

        if (!hasMRTViewport)
        {
            mrtViewport.TopLeftX = 0.f;
            mrtViewport.TopLeftY = 0.f;
            mrtViewport.Width = static_cast<float>(renderTarget->Get_Width());
            mrtViewport.Height = static_cast<float>(renderTarget->Get_Height());
            mrtViewport.MinDepth = 0.f;
            mrtViewport.MaxDepth = 1.f;
            hasMRTViewport = true;
        }

        renderTargets[iNumRenderTargets++] = renderTarget->Get_RTV();
    }

    _context->OMSetRenderTargets(iNumRenderTargets, renderTargets, shaderDSV.Get() ? shaderDSV.Get() : _originalDSV.Get());

    if (hasMRTViewport)
        _context->RSSetViewports(1, &mrtViewport);
    
    return S_OK;

}

HRESULT Target_Manager::End_MRT()
{
    ID3D11RenderTargetView* backBufferRaw = _backBuffer.Get();
    _context->OMSetRenderTargets(1, &backBufferRaw, _originalDSV.Get());

    if (_originalViewportCount > 0)
    {
        _context->RSSetViewports(_originalViewportCount, _originalViewports);
        _originalViewportCount = 0;
    }

    _backBuffer.Reset();
    _originalDSV.Reset();
    
    return S_OK;
}

#ifdef _DEBUG
HRESULT Target_Manager::Ready_Debug(const wstring& targetTag, float x, float y, float sizeX, float sizeY)
{
    auto renderTarget = Find_RenderTarget(targetTag);
    CHECK_NULL(renderTarget, E_FAIL);

    return renderTarget->Ready_Debug(x, y, sizeX, sizeY);
}

HRESULT Target_Manager::Render_Debug(const wstring& mrtTag, Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer)
{
    auto mrtList = Find_MRT(mrtTag);
    CHECK_NULL(mrtList, E_FAIL);

    for (auto renderTarget : *mrtList)
    {
        renderTarget->Render(viBuffer, shader);
    }

    return S_OK;
}

HRESULT Target_Manager::Log_DebugFloatStats(const wstring& targetTag)
{
    auto renderTarget = Find_RenderTarget(targetTag);
    CHECK_NULL(renderTarget, E_FAIL);

    return renderTarget->Log_DebugFloatStats(targetTag);
}
#endif

Shared<RenderTarget> Target_Manager::Find_RenderTarget(const wstring& targetTag)
{
    auto iter = _renderTargets.find(targetTag);

    if (iter == _renderTargets.end())
        return nullptr;

    return iter->second.target;
}

list<Shared<RenderTarget>>* Target_Manager::Find_MRT(const wstring& mrtTag)
{
    auto iter = _MRTs.find(mrtTag);
    if (iter == _MRTs.end())
        return nullptr;

    return &iter->second;
}

Unique<Target_Manager> Target_Manager::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    return make_unique<Target_Manager>(device, context);
}

void Target_Manager::Free()
{
    Base::Free();
}
