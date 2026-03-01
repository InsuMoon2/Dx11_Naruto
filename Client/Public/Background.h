#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Background final : public UIObject
{
    GENERATED_BODY(Background)

public:
    struct FBackgroundDesc final : public UIObject::FUIDesc
    {
        uint32 levelIndex = 0;
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;
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
    HRESULT     Ready_Components();

private:
    Shared<Texture>         _textureCom;
    Shared<Shader>          _shaderCom;
    Shared<VIBuffer_Rect>   _bufferCom;

private:
    uint32                  _levelIndex = 0;
    uint32                  _textureIndex = 0;
    uint32                  _textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

public:
    static Shared<UIObject>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject>          Clone(void* arg) override;
    virtual void                Free() override;
    
};

NS_END
