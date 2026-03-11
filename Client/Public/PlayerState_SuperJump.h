#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_SuperJump : public IPlayerState
{
public:
    PlayerState_SuperJump();
    ~PlayerState_SuperJump() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Jump; }

public:
    static Shared<PlayerState_SuperJump> Create();
};

NS_END
