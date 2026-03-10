#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL ModelMaterial : public Base
{
    GENERATED_BODY(ModelMaterial)

public:
    explicit ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~ModelMaterial() = default;

public:
    HRESULT Initialize_FromJson(const json& data, const string& materialFilePath);
    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

    string  Get_MaterialName() const { return _materialName; }
    uint32  Get_TextureCount(EMaterialTextureSlot slot) const;
    string  Get_TextureGuid(EMaterialTextureSlot slot, uint32 index) const;

    HRESULT Override_Texture(EMaterialTextureSlot slot, uint32 index, const string& guid);

public:
    json    To_Json() const;
    void    From_Json(const json& data);

private:
    static EMaterialTextureSlot SlotString_To_Enum(const string& slot);
    static string SlotEnum_To_String(EMaterialTextureSlot slot);

    HRESULT Load_Texture_File(EMaterialTextureSlot slot, uint32 index, const string& texturePath, const string& baseFilePath);
    HRESULT Bind_Texture_Internal(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

private:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

    vector<ComPtr<ShaderResourceView>> _textures[MATERIAL_TEXTURE_SLOT_COUNT];
    vector<string>                     _textureGuids[MATERIAL_TEXTURE_SLOT_COUNT];

    string                             _materialName;


public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const json& data, const string& materialFilePath);

    void Free() override;
    
};

NS_END
