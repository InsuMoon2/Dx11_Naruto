#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_WireDash : public IPlayerState
{
public:
    PlayerState_WireDash();
    ~PlayerState_WireDash() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::WireDash; }

private:
    bool _isWireAttach = false;
    Vec3 _wireAttachPosition = Vec3::Zero;

public:
    static Shared<PlayerState_WireDash> Create();
};

NS_END
