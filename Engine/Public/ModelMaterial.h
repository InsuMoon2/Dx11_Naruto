#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL ModelMaterial : public Base
{
    GENERATED_BODY(ModelMaterial)

private:
    struct FMaterialTextureMeta
    {
        // [추가] 이 텍스처가 어느 UV 채널을 써야 하는지 나타내는 인덱스다.
        uint32 uvChannel = 0;
        // [추가] 원본 머티리얼의 sampling scale을 복원한 값이다.
        float samplingScale = 1.f;
    };

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
    // [추가] 셰이더가 texture별로 올바른 UV 세트를 고를 수 있도록 UV 채널 메타를 제공한다.
    uint32  Get_TextureUVChannel(EMaterialTextureSlot slot, uint32 index) const;
    // [추가] 셰이더가 원본 타일링을 복원할 수 있도록 texture별 sampling scale을 제공한다.
    float   Get_TextureSamplingScale(EMaterialTextureSlot slot, uint32 index) const;

    HRESULT Override_Texture(EMaterialTextureSlot slot, uint32 index, const string& guid);

public:
    const Vec4&     Get_BaseColorFactor() const { return _baseColorFactor; }
    float           Get_NormalStrength() const { return _normalStrength; }
    const string&   Get_MaterialProfile() const { return _materialProfile; }

    static Vec4 Read_Vec4_Array(const json& data, const char* key, const Vec4& defaultValue);

public: /* 머티리얼 종류 추가 */
    float           Get_BlendNormalStrength() const { return _blendNormalStrength; }
    float           Get_MaskScale() const { return _maskScale; }
    float           Get_MaskThreshold() const { return _maskThreshold; }
    float           Get_UnevenColorScale() const { return _unevenColorScale; }
    Vec4            Get_ShadowColor() const { return _shadowColor; }

public:
    json    To_Json() const;
    void    From_Json(const json& data);

private:
    static EMaterialTextureSlot SlotString_To_Enum(const string& slot);
    static string SlotEnum_To_String(EMaterialTextureSlot slot);

    // [추가] slot/index 조합에 대응하는 texture 메타 배열 크기를 미리 맞춘다.
    void    Ensure_TextureMetaStorage(EMaterialTextureSlot slot, uint32 index);
    // [추가] JSON에서 읽은 UV 채널과 sampling scale을 slot/index 기준으로 저장한다.
    void    Set_TextureMeta(EMaterialTextureSlot slot, uint32 index, uint32 uvChannel, float samplingScale);
    HRESULT Load_Texture_File(EMaterialTextureSlot slot, uint32 index, const string& texturePath, const string& baseFilePath);
    HRESULT Bind_Texture_Internal(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

private:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

    vector<ComPtr<ShaderResourceView>> _textures[MATERIAL_TEXTURE_SLOT_COUNT];
    vector<string>                     _textureGuids[MATERIAL_TEXTURE_SLOT_COUNT];
    // [추가] 각 텍스처 슬롯/인덱스가 사용하는 UV 채널과 타일링 메타를 저장한다.
    vector<FMaterialTextureMeta>       _textureMetas[MATERIAL_TEXTURE_SLOT_COUNT];

    string                             _materialName;
    string                             _materialInstanceGuid;

private:
    // Mateiral Instnace
    Vec4    _baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
    Vec4    _shadowColor = Vec4(1.f, 1.f, 1.f, 1.f);
    float   _normalStrength = 1.f;
    string  _materialProfile;
    int32   _blendMode = 0;

    float   _blendNormalStrength = 1.f;
    float   _maskScale = 1.f;
    float   _maskThreshold = 1.f;
    float   _unevenColorScale = 1.f;

public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const json& data, const string& materialFilePath);

    void Free() override;
    
};

NS_END
