#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_SuperJumpCharge : public IPlayerState
{
public:
    PlayerState_SuperJumpCharge();
    ~PlayerState_SuperJumpCharge() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::SuperJumpCharge; }

private:
    bool    _loopAnimStarted = false;

public:
    static Shared<PlayerState_SuperJumpCharge> Create();
};

NS_END
