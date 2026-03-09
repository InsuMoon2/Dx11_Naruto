#include "pch.h"
#include "ModelMaterial.h"

#include "Shader.h"

ModelMaterial::ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{

}

HRESULT ModelMaterial::Initialize(const aiMaterial* aiMaterial, const string& modelFilePath)
{
    for (uint32 typeIndex = 0; typeIndex < AI_TEXTURE_TYPE_MAX; ++typeIndex)
    {
        aiTextureType type = static_cast<aiTextureType>(typeIndex);
        uint32 numTextures = aiMaterial->GetTextureCount(type);

        for (uint32 i = 0; i < numTextures; ++i)
        {
            aiString texturePath;
            if (aiMaterial->GetTexture(type, i, &texturePath) != AI_SUCCESS)
                continue;

            string fullPath = texturePath.C_Str();

            // 상대경로인 경로 모델 디렉토리 기준으로 조합
            fs::path texPath = fullPath;
            if (texPath.is_relative())
            {
                fs::path modelDir = fs::path(modelFilePath).parent_path();
                fullPath = (modelDir / texPath).string();
            }

            // SRV
            wstring wPath = Utils::ToWString(fullPath);
            ComPtr<ShaderResourceView> srv;

            wstring ext = fs::path(wPath).extension().wstring();
            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            HRESULT hr;

            if (ext == L".dds")
                hr = DirectX::CreateDDSTextureFromFile(
                    _device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());

            else
                hr = DirectX::CreateWICTextureFromFile(
                    _device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());

            if (FAILED(hr))
            {
                LOG_WARN("ModelMaterial: Failed to load texture - {}", fullPath);
                continue;
            }

            _textures[typeIndex].push_back(srv);
        }

    }

    return S_OK;
}

HRESULT ModelMaterial::Bind_Material(Shared<Shader> shader, const char* constantName, aiTextureType type, uint32 textureIndex)
{
    uint32 typeIndex = static_cast<uint32>(type);

    if (typeIndex >= AI_TEXTURE_TYPE_MAX)
        return E_FAIL;

    if (textureIndex >= _textures[typeIndex].size())
        return E_FAIL;

    return shader->Bind_SRV(constantName, _textures[typeIndex][textureIndex]);
}

Shared<ModelMaterial> ModelMaterial::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const aiMaterial* aiMaterial, const string& modelFilePath)
{
    auto instance = make_shared<ModelMaterial>(device, context);

    if (FAILED(instance->Initialize(aiMaterial, modelFilePath)))
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
