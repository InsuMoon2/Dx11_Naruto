#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Texture;
class Shader;
class VIBuffer_Rect;
class Controller;

class ENGINE_DLL Character abstract : public GameObject
{
    GENERATED_BODY(Character)

public:
    struct FCharacterDesc : public FGameObjectDesc
    {
        // Temp
        float moveSpeed = 5.f;
        float turnSpeed = 360.f;
    };

public:
    explicit Character(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Character(const Character& rhs);
    virtual ~Character();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
	void	BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    virtual HRESULT Bind_Lights();

protected:
    virtual HRESULT Ready_Components();

protected:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

public:
    void Free() override;
};

NS_END
