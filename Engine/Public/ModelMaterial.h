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
    HRESULT Initialize(const aiMaterial* aiMaterial, const string& modelFilePath);
    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName, aiTextureType type, uint32 textureIndex);

private:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

    vector<ComPtr<ShaderResourceView>> _textures[AI_TEXTURE_TYPE_MAX];

public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const aiMaterial* aiMaterial, const string& modelFilePath);

    void Free() override;
    

    
};

NS_END
