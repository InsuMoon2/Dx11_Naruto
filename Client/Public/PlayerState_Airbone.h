#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Airbone : public IPlayerState
{
public:
    PlayerState_Airbone();
    ~PlayerState_Airbone() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Attack_Airbone; }

private:
    bool _startedInAir = false;

public:
    static Shared<PlayerState_Airbone> Create();
};

NS_END
