#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Shuriken : public IPlayerState
{
public:
    PlayerState_Shuriken();
    ~PlayerState_Shuriken() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Shuriken; }

private:

public:
    static Shared<PlayerState_Shuriken> Create();
};

NS_END
