#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Dash : public IPlayerState
{
public:
    PlayerState_Dash();
    ~PlayerState_Dash() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Dash; }

private:
    static EMoveInputDirection Classify_InputDirection(const Vec2& moveAxis);

private:
    EMoveInputDirection _dashDir = EMoveInputDirection::Forward;


public:
    static Shared<PlayerState_Dash> Create();
};

NS_END
