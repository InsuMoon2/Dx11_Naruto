#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
    class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class TargetComponent;

class UI_Targeting : public UIObject
{
    GENERATED_BODY(UI_Targeting)

public:
    struct FUITargetingDesc : public FUIDesc
    {
        uint32 textureIndex = 0;
    };

    enum class ETargetingTexture { TargetBase, TargetLock, };

public:
    explicit UI_Targeting(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_Targeting(const UI_Targeting& rhs);
    virtual ~UI_Targeting() = default;

public:
    virtual HRESULT     Initialize_Prototype() override;
    virtual HRESULT     Initialize(void* arg) override;

    virtual void        Priority_Update(float timeDelta) override;
    virtual void        Update(float timeDelta) override;
    virtual void        Late_Update(float timeDelta) override;

    virtual HRESULT     Render() override;

public:
    void    Set_TargetComponent(Weak<TargetComponent> targetCom) { _targetCom = targetCom; }

protected:
    HRESULT Ready_Components() override;

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

    Weak<TargetComponent>   _targetCom;

public:
    static Shared<UI_Targeting> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject>  Clone(void* arg) override;
    virtual void                Free() override;
};

NS_END
