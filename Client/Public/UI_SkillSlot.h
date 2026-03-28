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
        uint32 maskSrvIndex = 1; // 마스킹용 텍스처
        uint32 iconSrvIndex = 2;

        uint32 textureComponentType = Protocol::COMPONENT_TYPE_TEXTURE_SKILL_ICON;
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
    void Set_MaskSrvIndex(uint32 index) { _maskSrvIndex = index; }
    void Set_SrvIndex(uint32 index)     { _iconSrvIndex = index; }
    void Set_CooldownRatio(float ratio) { _cooldownRatio = ::clamp(ratio, 0.f, 1.f); }

private:
    HRESULT Bind_CommonShaderResources();
    HRESULT Render_Base();          // 스킬 칸 베이스로 렌더링, 스킬 아이콘 짤리게
    HRESULT Render_MaskedIcon();    // Base 알파를 마스크로 사용해서 원형 내부에만 아이콘 보이게
    HRESULT Render_MaskedCooldwn(); // 쿨타임 오버레이도 Base 안에서만 렌더되게

protected:
    HRESULT Ready_Components() override;

private:
    uint32  _baseSrvIndex = 0;
    uint32  _maskSrvIndex = 1;
    uint32  _iconSrvIndex = 2;

    float   _cooldownRatio = 0.f;
    float   _cooldownOverlayAlpha = 0.55f;

    uint32  _textureComponentType = Protocol::COMPONENT_TYPE_TEXTURE_SKILL_ICON;

private:
    Shared<Shader>        _shaderCom;

    Shared<Texture>       _gaugeTextureCom;
    Shared<Texture>       _iconTextureCom;

    Shared<VIBuffer_Rect> _bufferCom;

public:
    static Shared<UI_SkillSlot> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
