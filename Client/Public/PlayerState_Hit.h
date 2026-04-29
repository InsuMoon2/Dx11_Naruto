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

private:
    // 플레이어 피격은 복잡한 리액션 타입보다 현재 지면 상태를 우선해서 안정적으로 Hit / Hit_Air를 고른다.
    static EPlayerState Resolve_HitState(const Shared<MovementComponent>& movement);

public:
    static Shared<PlayerState_Hit> Create();
};

NS_END
