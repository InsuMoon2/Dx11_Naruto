#pragma once

#include "Panel.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Rect;
class UI_Text;
NS_END

NS_BEGIN(Client)

class Player;
class UI_PlayerHP;
class CombatStat;

class UI_PlayerStatus : public Panel
{
    GENERATED_BODY(UI_PlayerStatus)

public:
    explicit UI_PlayerStatus(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerStatus(const UI_PlayerStatus& rhs);
    virtual ~UI_PlayerStatus() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

public:
    void        Bind_Player(Shared<Player> player);

protected:
    HRESULT     Ready_Components();

private:
    Weak<Player>            _player;
    Weak<CombatStat>        _combat;

    Shared<UI_PlayerHP>     _hpBar;


private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;


public:
    static Shared<UI_PlayerStatus> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg);
    void Free() override;
};

NS_END
