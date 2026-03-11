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
    HRESULT Initialize_FromMaterialInstance(const string& matInstanceFilePath);
    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

    string  Get_MaterialName() const { return _materialName; }
    uint32  Get_TextureCount(EMaterialTextureSlot slot) const;
    string  Get_TextureGuid(EMaterialTextureSlot slot, uint32 index) const;

    HRESULT Override_Texture(EMaterialTextureSlot slot, uint32 index, const string& guid);

public:
    const Vec4&     Get_BaseColorFactor() const { return _baseColorFactor; }
    float           Get_NormalStrength() const { return _normalStrength; }
    const string&   Get_MaterialProfile() const { return _materialProfile; }

    static Vec4 Read_Vec4_Array(const json& data, const char* key, const Vec4& defaultValue);

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
    string                             _materialInstanceGuid;

private:
    // Mateiral Instnace
    Vec4    _baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
    Vec4    _shadowColor = Vec4(1.f, 1.f, 1.f, 1.f);
    float   _normalStrength = 1.f;
    string  _materialProfile;
    int32   _blendMode = 0;

public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const json& data, const string& materialFilePath);

    void Free() override;
    
};

NS_END
