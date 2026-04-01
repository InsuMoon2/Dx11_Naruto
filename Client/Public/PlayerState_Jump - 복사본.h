#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_JumpFall : public IPlayerState
{
public:
    PlayerState_JumpFall();
    ~PlayerState_JumpFall() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::JumpFall; }

public:
    static Shared<PlayerState_JumpFall> Create();
};

NS_END
