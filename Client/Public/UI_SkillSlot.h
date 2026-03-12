#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
    class Shader;
class Texture;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_SkillSlot final : public UIObject
{
    GENERATED_BODY(UI_SkillSlot)

public:
    struct FSkillSlotDesc : public UIObject::FUIDesc
    {
        uint32 baseSrvIndex = 0; // 스킬 세팅 안됐을 때
        uint32 iconSrvIndex = 2;
    };

public:
    explicit UI_SkillSlot(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_SkillSlot(const UI_SkillSlot& rhs);
    virtual ~UI_SkillSlot() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Set_BaseSrvIndex(uint32 index) { _baseSrvIndex = index; }
    void Set_SrvIndex(uint32 index) { _iconSrvIndex = index; }
    void Set_CooldownRatio(float ratio) { _cooldownRatio = ::clamp(ratio, 0.f, 1.f); }

protected:
    HRESULT Ready_Components() override;

private:
    uint32  _baseSrvIndex = 0;
    uint32  _iconSrvIndex = 2;

    float   _cooldownRatio = 0.f;
    float   _cooldownOverlayAlpha = 0.55f;

private:
    Shared<Shader>        _shaderCom;
    Shared<Texture>       _textureCom;
    Shared<VIBuffer_Rect> _bufferCom;

public:
    static Shared<UI_SkillSlot> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg = nullptr);
    void Free() override;

};

NS_END
