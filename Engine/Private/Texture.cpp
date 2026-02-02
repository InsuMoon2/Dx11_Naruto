#include "pch.h"
#include "Texture.h"

Texture::Texture(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Texture::Texture(const Texture& rhs)
    : Component(rhs)
    , _numSRVs(rhs._numSRVs)
    , _SRVs(rhs._SRVs)
{
    
}

Texture::~Texture()
{
}

HRESULT Texture::Initialize_Prototype(const wstring& texturePath, uint32 numSRVs)
{
    _numSRVs = numSRVs;

    for (uint32 i = 0; i < numSRVs; i++)
    {
        wchar_t fullPath[MAX_PATH] = {};
        wsprintf(fullPath, texturePath.c_str(), i);

        // 확장자 확인
        wchar_t ext[MAX_PATH] = {};
        _wsplitpath_s(fullPath, nullptr, 0, nullptr, 0, nullptr, 0, ext, MAX_PATH);

        ComPtr<ShaderResourceView> srv;
        HRESULT hr = {};

        if (wcscmp(ext, TEXT(".dds")) == 0)
            hr = CreateDDSTextureFromFile(_device.Get(), fullPath, nullptr, srv.GetAddressOf());

        else if (wcscmp(ext, TEXT(".tga")) == 0)
            return E_FAIL;

        else
            hr = CreateWICTextureFromFile(_device.Get(), fullPath, nullptr, srv.GetAddressOf());

        CHECK_FAILED_RETURN(hr, E_FAIL);

        _SRVs.emplace_back(srv);
    }

    return S_OK;
}

HRESULT Texture::Initialize(any arg)
{
    return Component::Initialize(arg);
}

shared_ptr<Texture> Texture::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& texturePath, uint32 numSRVs)
{
    auto instance = make_shared<Texture>(device, context);

    if (FAILED(instance->Initialize_Prototype(texturePath, numSRVs)))
    {
        MSG_BOX("Failed to Created : Texture");

        return nullptr;
    }

    

    return instance;
}

shared_ptr<Component> Texture::Clone(any arg)
{
    auto instance = make_shared<Texture>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Texture");

        return nullptr;
    }

    return instance;
}

void Texture::Free()
{
    Component::Free();

    _SRVs.clear();
}
