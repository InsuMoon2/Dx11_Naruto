#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Monster;
class CombatStat;

class UI_MonsterHp : public UIObject
{
    GENERATED_BODY(UI_MonsterHp)

public:
    struct FPlayerHPDesc : public Engine::UIObject::FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;
    };

public:
    explicit UI_MonsterHp(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_MonsterHp(const UI_MonsterHp& rhs);
    virtual ~UI_MonsterHp() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

public:
    void        Set_Ratio(float ratio) { _hpRatio = ::clamp(ratio, 0.f, 1.f); }
    void        Set_FillRange(float startU, float endU) { _fillStartU = startU; _fillEndU = endU; }

    void        Bind_Monster(Shared<Monster> monster);

protected:
    virtual HRESULT Ready_Components() override;

private:
    Shared<Shader>        _shaderCom;
    Shared<Texture>       _textureCom;
    Shared<VIBuffer_Rect> _bufferCom;

private:
    float _hpRatio = 1.f;

    float _fillStartU = 0.f;
    float _fillEndU = 1.f;
    bool  _fillRangeInitialized = false;

    Weak<Monster>       _monster;
    Weak<CombatStat>    _combat;

    
public:
    static Shared<UI_MonsterHp> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
