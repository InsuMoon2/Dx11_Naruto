#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_JumpDash : public IPlayerState
{
public:
    PlayerState_JumpDash();
    ~PlayerState_JumpDash() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::JumpDash; }

private:
    EMoveInputDirection _dashDir = EMoveInputDirection::Forward;

    bool _requestedAnimEnd = false;
    bool _gravityRestored = false;

public:
    static Shared<PlayerState_JumpDash> Create();
};

NS_END
