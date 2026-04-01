#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Hit : public IPlayerState
{
public:
    PlayerState_Hit() = default;
    ~PlayerState_Hit() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Hit; }

public:
    static Shared<PlayerState_Hit> Create();
};

NS_END
