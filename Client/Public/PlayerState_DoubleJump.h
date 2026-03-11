#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_DoubleJump : public IPlayerState
{
public:
    PlayerState_DoubleJump();
    ~PlayerState_DoubleJump() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Jump; }

public:
    static Shared<PlayerState_DoubleJump> Create();
};

NS_END
