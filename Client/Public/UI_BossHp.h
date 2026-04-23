#pragma once

#include "HUD.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Background;
class CombatStat;

class UI_BossHp final : public HUD
{
    GENERATED_BODY(UI_BossHp)

public:
    explicit UI_BossHp(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_BossHp(const UI_BossHp& rhs);
    virtual ~UI_BossHp() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Bind_Boss(Shared<GameObject> bossObject);
    void    Clear_Boss();

    void    Set_FillRange(float startU, float endU) { _fillStartU = startU; _fillEndU = endU; }

protected:
    HRESULT Ready_Components() override;

private:
    HRESULT Ready_UI();
    void    Update_BossBinding(float timeDelta);
    void    Begin_IntroFill(float targetHpRatio);

private:
    Shared<Shader>        _shaderCom;
    Shared<Texture>       _textureCom;
    Shared<VIBuffer_Rect> _bufferCom;

    Weak<GameObject>      _bossObject;
    Weak<CombatStat>      _bossCombat;

    float                 _targetHpRatio = 1.f;
    float                 _displayHpRatio = 1.f;

    float                 _hpRatio = 1.f;
    float                 _fillStartU = 0.f;
    float                 _fillEndU = 1.f;
    Color                 _fillColor = Color(0.95f, 0.22f, 0.08f, 1.f);

    float                 _introStartHpRatio = 0.6f;
    float                 _introFillDuration = 0.9f;
    float                 _introElapsed = 0.f;
    bool                  _isIntroPlaying = false;


public:
    static Shared<UI_BossHp> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
