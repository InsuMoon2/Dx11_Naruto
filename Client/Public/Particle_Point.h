#pragma once

#include "GameObject.h"
#include "VIBuffer_Particle_Point.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
NS_END

NS_BEGIN(Client)

enum class EParticleTextureType
{
    Hit, 
};

class Particle_Point : public GameObject
{
    GENERATED_BODY(Particle_Point)

public:
    struct FParticlePointDesc : public GameObject::FGameObjectDesc
    {
        uint32 shaderType = Protocol::COMPONENT_TYPE_SHADER_PARTICLE_POINT;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW;

        uint32 textureIndex = 0;

        VIBuffer_Particle_Point::FParticlePointDesc bufferDesc{};

        bool addToBlendGroup = false;
    };

public:
    explicit Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Particle_Point(const Particle_Point& rhs);
    virtual ~Particle_Point() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;
    HRESULT Bind_ShaderResources() override;

private:
    HRESULT Ready_Components(const FParticlePointDesc& desc);

private:
    Shared<Shader>  _shaderCom;
    Shared<Texture> _textureCom;
    Shared<VIBuffer_Particle_Point> _bufferCom;

    uint32  _textureIndex = 0;   
    bool    _addToBlendGroup = true;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
