#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Background final : public GameObject
{
    GENERATED_BODY(Background)

public:
    struct FBackgroundDesc final : public GameObject::FGameObjectDesc
    {
        int32 flag = 0;
    };

public:
    explicit Background(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Background(const Background& rhs);
    virtual ~Background();

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

private:
    HRESULT Ready_Components();

private:
    Shared<Texture>         _textureCom;
    Shared<Shader>          _shaderCom;
    Shared<VIBuffer_Rect>   _bufferCom;

public:
    static shared_ptr<GameObject>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject>          Clone(void* arg) override;
    virtual void                    Free() override;
    
};

NS_END
