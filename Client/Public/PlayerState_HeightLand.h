#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_HeightLand : public IPlayerState
{
public:
    PlayerState_HeightLand();
    ~PlayerState_HeightLand() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::HeightLand; }

private:
    Vec3    _rollDirection = Vec3::Zero;

    float   _endHoldTime = 0.f;
    float   _moveSpeed = 15.f;

public:
    static Shared<PlayerState_HeightLand> Create();
};

NS_END
