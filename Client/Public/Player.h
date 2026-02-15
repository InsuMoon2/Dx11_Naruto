#pragma once

#include "Character.h"

NS_BEGIN(Client)

class CombatStat;
class PlayerController;
class InputComponent;
class MovementComponent;

class Player final : public Character
{
    GENERATED_BODY(Player)

public:
    explicit Player(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Player(const Player& rhs);
    virtual ~Player() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    BeginPlay() override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Components() override;

private:
    Shared<CombatStat>          _combatStat;
    Shared<InputComponent>      _input;
    Shared<MovementComponent>   _movement;
    Shared<PlayerController>    _playerController;

public:
    static shared_ptr<GameObject>  Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual shared_ptr<GameObject> Clone(void* arg) override;

};

NS_END
