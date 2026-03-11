#include "pch.h"
#include "ModelMaterial.h"
#include "GameInstance.h"
#include "Shader.h"
#include <fstream>

ModelMaterial::ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{

}

HRESULT ModelMaterial::Initialize_FromJson(const json& data, const string& materialFilePath)
{
    {
        _baseColorFactor = Read_Vec4_Array(data, "base_color_factor", Vec4(1.f, 1.f, 1.f, 1.f));
        _shadowColor = Read_Vec4_Array(data, "shadow_color", Vec4(1.f, 1.f, 1.f, 1.f));
        _normalStrength = data.value("normal_strength", 1.f);
        _materialProfile = data.value("profile", string(""));
        _blendMode = data.value("blend_mode", 0);
    }

    _materialName = data.value("material_name", data.value("name", string("Material")));

    if (!data.contains("textures") || !data["textures"].is_array())
        return S_OK;

    for (const auto& textureItem : data["textures"])
    {
        if (!textureItem.is_object())
            continue;

        string slotStr = textureItem.value("slot", "");
        uint32 index = textureItem.value("index", 0u);
        string path = textureItem.value("path", "");

        if (path.empty())
            continue;

        EMaterialTextureSlot slot = SlotString_To_Enum(slotStr);
        if (slot == EMaterialTextureSlot::END)
            continue;

        CHECK_FAILED(Load_Texture_File(slot, index, path, materialFilePath), E_FAIL);
    }

    return S_OK;
}

HRESULT ModelMaterial::Initialize_FromMaterialInstance(const string& matInstanceFilePath)
{
    ifstream file(matInstanceFilePath);
    if (!file.is_open())
        return E_FAIL;

    json root;
    file >> root;
    file.close();

    return Initialize_FromJson(root, matInstanceFilePath);
}

HRESULT ModelMaterial::Bind_Material(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex)
{
    return Bind_Texture_Internal(shader, constantName, slot, textureIndex);
}

uint32 ModelMaterial::Get_TextureCount(EMaterialTextureSlot slot) const
{
    const uint32 idx = static_cast<uint32>(slot);

    if (idx >= MATERIAL_TEXTURE_SLOT_COUNT)
        return 0;

    return static_cast<uint32>(_textures[idx].size());
}

string ModelMaterial::Get_TextureGuid(EMaterialTextureSlot slot, uint32 index) const
{
    const uint32 idx = static_cast<uint32>(slot);

    if (idx >= MATERIAL_TEXTURE_SLOT_COUNT || index >= _textureGuids[idx].size())
        return "";

    return _textureGuids[idx][index];
}

HRESULT ModelMaterial::Override_Texture(EMaterialTextureSlot slot, uint32 index, const string& guid)
{
    const uint32 idx = static_cast<uint32>(slot);
    if (idx >= MATERIAL_TEXTURE_SLOT_COUNT)
        return E_FAIL;

    wstring path = GAME->Resolve_AssetPath(guid);
    if (path.empty())
        return E_FAIL;

    ComPtr<ShaderResourceView> srv;
    wstring ext = fs::path(path).extension().wstring();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    HRESULT hr;
    if (ext == L".dds")
        hr = DirectX::CreateDDSTextureFromFile(_device.Get(), path.c_str(), nullptr, srv.GetAddressOf());
    else
        hr = DirectX::CreateWICTextureFromFile(_device.Get(), path.c_str(), nullptr, srv.GetAddressOf());

    if (FAILED(hr))
        return hr;

    if (_textures[idx].size() <= index)
        _textures[idx].resize(index + 1);

    if (_textureGuids[idx].size() <= index)
        _textureGuids[idx].resize(index + 1);

    _textures[idx][index] = srv;
    _textureGuids[idx][index] = guid;

    return S_OK;
}

EMaterialTextureSlot ModelMaterial::SlotString_To_Enum(const string& slot)
{
    if (slot == "base_color")         return EMaterialTextureSlot::BaseColor;
    if (slot == "normal")             return EMaterialTextureSlot::Normal;
    if (slot == "specular")           return EMaterialTextureSlot::Specular;
    if (slot == "emissive")           return EMaterialTextureSlot::Emissive;
    if (slot == "ambient_occlusion")  return EMaterialTextureSlot::AmbientOcclusion;
    if (slot == "metalness")          return EMaterialTextureSlot::Metalness;
    if (slot == "roughness")          return EMaterialTextureSlot::Roughness;

    return EMaterialTextureSlot::END;
}

string ModelMaterial::SlotEnum_To_String(EMaterialTextureSlot slot)
{
    switch (slot)
    {
    case EMaterialTextureSlot::BaseColor:         return "base_color";
    case EMaterialTextureSlot::Normal:            return "normal";
    case EMaterialTextureSlot::Specular:          return "specular";
    case EMaterialTextureSlot::Emissive:          return "emissive";
    case EMaterialTextureSlot::AmbientOcclusion:  return "ambient_occlusion";
    case EMaterialTextureSlot::Metalness:         return "metalness";
    case EMaterialTextureSlot::Roughness:         return "roughness";
    default:                                      return "";
    }
}

HRESULT ModelMaterial::Load_Texture_File(EMaterialTextureSlot slot, uint32 index, const string& texturePath,
    const string& baseFilePath)
{
    if (texturePath.empty())
        return S_OK;

    const uint32 slotIndex = static_cast<uint32>(slot);
    if (slotIndex >= MATERIAL_TEXTURE_SLOT_COUNT)
        return E_FAIL;

    fs::path finalPath(texturePath);
    if (finalPath.is_relative())
    {
        fs::path baseDir = fs::path(baseFilePath).parent_path();
        finalPath = baseDir / finalPath;
    }

    wstring wPath = fs::absolute(finalPath).wstring();
    wstring ext = fs::path(wPath).extension().wstring();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    ComPtr<ShaderResourceView> srv;
    HRESULT hr = E_FAIL;

    if (ext == L".dds")
        hr = DirectX::CreateDDSTextureFromFile(_device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());
    else
        hr = DirectX::CreateWICTextureFromFile(_device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());

    if (FAILED(hr))
        return S_OK;

    if (_textures[slotIndex].size() <= index)
        _textures[slotIndex].resize(index + 1);

    if (_textureGuids[slotIndex].size() <= index)
        _textureGuids[slotIndex].resize(index + 1);

    _textures[slotIndex][index] = srv;
    _textureGuids[slotIndex][index] = GAME->Find_AssetGUID(wPath);

    return S_OK;
}

Vec4 ModelMaterial::Read_Vec4_Array(const json& data, const char* key, const Vec4& defaultValue)
{
    if (!data.contains(key) || !data[key].is_array() || data[key].size() < 4)
        return defaultValue;

    return Vec4(
        data[key][0].get<float>(),
        data[key][1].get<float>(),
        data[key][2].get<float>(),
        data[key][3].get<float>()
    );
}

json ModelMaterial::To_Json() const
{
    json j;
    j["material_name"] = _materialName;

    json texOverrides = json::object();

    for (uint32 slotIndex = 0; slotIndex < MATERIAL_TEXTURE_SLOT_COUNT; ++slotIndex)
    {
        EMaterialTextureSlot slot = static_cast<EMaterialTextureSlot>(slotIndex);
        string slotName = SlotEnum_To_String(slot);

        if (slotName.empty())
            continue;

        for (uint32 texIndex = 0; texIndex < _textureGuids[slotIndex].size(); ++texIndex)
        {
            if (_textureGuids[slotIndex][texIndex].empty())
                continue;

            // CHANGED: 예전 "typeIndex_texIndex" 대신 "slot_index" 키 사용
            string key = slotName + "_" + to_string(texIndex);
            texOverrides[key] = _textureGuids[slotIndex][texIndex];
        }
    }

    if (!texOverrides.empty())
        j["texture_overrides"] = texOverrides;

    return j;
}

void ModelMaterial::From_Json(const json& data)
{
    if (!data.contains("texture_overrides"))
        return;

    auto& overrides = data["texture_overrides"];

    for (auto& [key, val] : overrides.items())
    {
        size_t sep = key.rfind('_');
        if (sep == string::npos)
            continue;

        string slotName = key.substr(0, sep);
        uint32 texIndex = stoi(key.substr(sep + 1));
        string guid = val.get<string>();

        if (guid.empty())
            continue;

        EMaterialTextureSlot slot = SlotString_To_Enum(slotName);
        if (slot == EMaterialTextureSlot::END)
            continue;

        const uint32 slotIndex = static_cast<uint32>(slot);

        // 이미 같은 GUID면 스킵
        if (texIndex < _textureGuids[slotIndex].size() &&
            _textureGuids[slotIndex][texIndex] == guid)
        {
            continue;
        }

        Override_Texture(slot, texIndex, guid);
    }
}

HRESULT ModelMaterial::Bind_Texture_Internal(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot,
    uint32 textureIndex)
{
    const uint32 slotIndex = static_cast<uint32>(slot);

    if (slotIndex >= MATERIAL_TEXTURE_SLOT_COUNT)
        return E_FAIL;

    if (textureIndex >= _textures[slotIndex].size())
        return E_FAIL;

    if (_textures[slotIndex][textureIndex] == nullptr)
        return E_FAIL;

    return shader->Bind_SRV(constantName, _textures[slotIndex][textureIndex]);
}

Shared<ModelMaterial> ModelMaterial::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const json& data, const string& materialFilePath)
{
    auto instance = make_shared<ModelMaterial>(device, context);

    if (FAILED(instance->Initialize_FromJson(data, materialFilePath)))
    {
        MSG_BOX("Failed to Create : ModelMaterial");
        return nullptr;
    }

    return instance;
}

void ModelMaterial::Free()
{
    for (auto& texArry : _textures)
    {
        texArry.clear();
    }

    Base::Free();
}
