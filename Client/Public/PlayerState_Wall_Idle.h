#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Wall_Idle : public IPlayerState
{
public:
    PlayerState_Wall_Idle();
    ~PlayerState_Wall_Idle() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Wall_Idle; }

public:
    static Shared<PlayerState_Wall_Idle> Create();
};

NS_END
