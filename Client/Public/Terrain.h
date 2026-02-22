#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Terrain;
NS_END

NS_BEGIN(Client)

class Terrain final : public GameObject
{
public:
    explicit Terrain(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Terrain(const Terrain& rhs);
    virtual ~Terrain() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Priority_Update(float fTimeDelta) override;
    void    Update(float fTimeDelta) override;
    void    Late_Update(float fTimeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Ready_Components();

private:
    Shared<Shader>              _shaderCom;
    Shared<Texture>             _textureCom;
    Shared<VIBuffer_Terrain>    _bufferCom;

public:
    static Shared<Terrain> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
