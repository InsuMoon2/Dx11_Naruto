#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_PlayerHP : public UIObject
{
    GENERATED_BODY(UI_PlayerHP)

public:
    struct FPlayerHPDesc : public Engine::UIObject::FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;
    };

public:
    explicit UI_PlayerHP(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerHP(const UI_PlayerHP& rhs);
    virtual ~UI_PlayerHP() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

public:
    void Set_Ratio(float ratio) { _hpRatio = ratio; }

protected:
    virtual HRESULT Ready_Components() override;

private:
    float _hpRatio = 1.f;

    Shared<Shader>        _shaderCom;
    Shared<Texture>       _textureCom;
    Shared<VIBuffer_Rect> _bufferCom;

public:
    static Shared<UI_PlayerHP> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg = nullptr);
    virtual void Free() override;
};

NS_END
