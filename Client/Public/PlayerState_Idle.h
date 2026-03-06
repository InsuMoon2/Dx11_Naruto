#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Idle : public IPlayerState
{
public:
    PlayerState_Idle();
    ~PlayerState_Idle() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Idle; }

public:
    static Shared<PlayerState_Idle> Create();
};

NS_END
