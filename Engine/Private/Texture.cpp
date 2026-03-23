#include "pch.h"
#include "Texture.h"
#include "Shader.h"
#include "GameInstance.h"

Texture::Texture(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Texture::Texture(const Texture& rhs)
    : Component(rhs)
    , _numSRVs(rhs._numSRVs)
    , _currentIndex(rhs._currentIndex)
    , _SRVs(rhs._SRVs)
    , _sourcePaths(rhs._sourcePaths)
    , _texturePath(rhs._texturePath)
{
    
}

Texture::~Texture()
{
}

HRESULT Texture::Initialize_Prototype(const wstring& texturePath, uint32 numSRVs)
{
    _texturePath = texturePath;
    _numSRVs = numSRVs;

    _SRVs.clear();
    _sourcePaths.clear();

    for (uint32 i = 0; i < numSRVs; i++)
    {
        wchar_t fullPath[MAX_PATH] = {};

        if (numSRVs == 1 && texturePath.find(L"%d") == wstring::npos)
            wcscpy_s(fullPath, texturePath.c_str());
        else
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
        {
            hr = CreateWICTextureFromFileEx(
                _device.Get(),
                fullPath,
                0, 
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE,
                0, 
                0, 
                DirectX::WIC_LOADER_IGNORE_SRGB,
                nullptr,
                srv.GetAddressOf()
            );
        }

        CHECK_FAILED(hr, E_FAIL);

        _SRVs.emplace_back(srv);
        _sourcePaths.emplace_back(fullPath);
    }

    return S_OK;
}

HRESULT Texture::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

json Texture::To_Json() const
{
    json j = Component::To_Json();

    string guid = GAME->Find_AssetGUID(_texturePath);
    if (!guid.empty())
    {
        j["texture_guid"] = guid;
    }
    else
    {
        j["texture_path"] = Utils::ToString(_texturePath);
    }

    j["num_srvs"] = _numSRVs;
    j["current_index"] = _currentIndex;

    return j;
}

void Texture::From_Json(const json& data)
{
    Component::From_Json(data);

    if (data.contains("current_index"))
        _currentIndex = data["current_index"].get<uint32>();

    wstring loadPath = L"";

    // 경로가 아닌, GUID 기반으로 로드 시도
    if (data.contains("texture_guid"))
    {
        string guid = data["texture_guid"].get<string>();
        wstring resolvePath = GAME->Resolve_AssetPath(guid);

        if (!resolvePath.empty())
        {
            loadPath = resolvePath;
        }
        else
        {
            LOG_ERROR("텍스처 로드 실패: GUID에 해당하는 에셋을 찾을 수 없음 - {}", guid);
        }
    }
    // 경로 기반 로드 시도 (이전에 남아있던 데이터들 때문에)
    else if(data.contains("texture_path"))
    {
        loadPath = Utils::ToWString(data["texture_path"].get<string>());
    }

    // 텍스처 초기화
    if (!loadPath.empty())
    {
        uint32 newCount = data.value("num_srvs", 1u);

        // 이미 같은 텍스처 로드 시 스킵
        if (_texturePath == loadPath && _numSRVs == newCount && !_SRVs.empty())
        {
            LOG_INFO("이미 텍스처 로드 완료, 스킵");
            return;
        }

        _texturePath = loadPath;
        _numSRVs = newCount;

        _SRVs.clear();
        Initialize_Prototype(_texturePath, _numSRVs);
    }
}

HRESULT Texture::Bind_SRV(Shared<Shader> shader, const char* constantName, uint32 index)
{
    if (index >= _numSRVs)
        return E_FAIL;

    return shader->Bind_SRV(constantName, _SRVs[index]);
}

HRESULT Texture::Add_SRV(const wstring& filePath)
{
    for (const auto& existPath : _sourcePaths)
    {
        if (existPath == filePath)
        {
            return S_OK;
        }
    }

    // 확장자 확인
    wchar_t ext[MAX_PATH] = {};
    _wsplitpath_s(filePath.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, ext, MAX_PATH);

    ComPtr<ShaderResourceView> srv;
    HRESULT hr = {};

    if (wcscmp(ext, TEXT(".dds")) == 0)
        hr = CreateDDSTextureFromFile(_device.Get(), filePath.c_str(), nullptr, srv.GetAddressOf());

    else if (wcscmp(ext, TEXT(".tga")) == 0)
        return E_FAIL;

    else
    {
        hr = CreateWICTextureFromFileEx(
            _device.Get(),
            filePath.c_str(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE,
            0,
            0,
            DirectX::WIC_LOADER_IGNORE_SRGB,
            nullptr,
            srv.GetAddressOf()
        );
    }

    CHECK_FAILED(hr, E_FAIL);

    _SRVs.emplace_back(srv);
    _sourcePaths.emplace_back(filePath);
    _numSRVs = static_cast<uint32>(_SRVs.size());

    return S_OK;
}

wstring Texture::Get_SourcePath(uint32 index) const
{
    if (index >= _sourcePaths.size())
        return L"";

    return _sourcePaths[index];
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

shared_ptr<Component> Texture::Clone(void* arg)
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
