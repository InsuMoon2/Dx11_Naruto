#include "pch.h"
#include "Target_Manager.h"
#include "RenderTarget.h"
#include "Shader.h"

Target_Manager::Target_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT Target_Manager::Add_RenderTarget(const wstring& targetTag, uint32 sizeX, uint32 sizeY, DXGI_FORMAT pixelFormat, const Color& clearColor)
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

    _renderTargets.emplace(targetTag, desc);

    _currentWidth = sizeX;
    _currentHeight = sizeY;

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

HRESULT Target_Manager::Resize_MRTs(uint32 width, uint32 height)
{
    if (_currentWidth == width && _currentHeight == height)
        return S_OK;

    if (width == 0 || height == 0) 
        return S_OK;

    for (auto& pair : _renderTargets)
    {
        FTargetDesc& desc = pair.second;
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

    _context->OMGetRenderTargets(1, _backBuffer.ReleaseAndGetAddressOf(), _originalDSV.ReleaseAndGetAddressOf());

    ID3D11RenderTargetView* renderTargets[8] = { nullptr };
    uint32 iNumRenderTargets = 0;

    for (auto& renderTarget : *mrtList)
    {
        auto findIter = _renderTargets.begin();

        for (auto & desc : _renderTargets)
        {
            if (desc.second.target == renderTarget)
            {
                // 그리기 전에 검은색 혹은 지정된 색으로 타겟 초기화
                renderTarget->Clear(desc.second.clearColor);
                break;
            }
        }

        renderTargets[iNumRenderTargets++] = renderTarget->Get_RTV();
    }

    _context->OMSetRenderTargets(iNumRenderTargets, renderTargets, shaderDSV.Get() ? shaderDSV.Get() : _originalDSV.Get());    
    
    return S_OK;

}

HRESULT Target_Manager::End_MRT()
{
    ID3D11RenderTargetView* backBufferRaw = _backBuffer.Get();
    _context->OMSetRenderTargets(1, &backBufferRaw, _originalDSV.Get());
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
