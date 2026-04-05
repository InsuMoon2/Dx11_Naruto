#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_WallRun : public IPlayerState
{
public:
    PlayerState_WallRun();
    ~PlayerState_WallRun() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Wall_Run; }

private:
    bool _isAttaching = false;

public:
    static Shared<PlayerState_WallRun> Create();
};

NS_END
