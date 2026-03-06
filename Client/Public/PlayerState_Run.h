#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Run : public IPlayerState
{
public:
    PlayerState_Run();
    ~PlayerState_Run() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Run; }

public:
    static Shared<PlayerState_Run> Create();
};

NS_END
