#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Replacement : public IPlayerState
{
public:
    PlayerState_Replacement() = default;
    ~PlayerState_Replacement() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Replacement; }

    bool Has_SuperArmor() const override { return true; }

    // 도착위치 계산
    bool Prepare_Replacement(PlayerStateMachine* state);

private:
    // 입력 방향 기준
    Vec3 Compute_BaseDirection(PlayerStateMachine* state);

    bool Try_FindReplacementDestination(
        PlayerStateMachine* state,
        const Vec3& baseDirection,
        Vec3& outDestination,
        Vec3& outLandingDirection) const;

    bool Validate_GroundCandidate(
        PlayerStateMachine* state,
        const Vec3& candidatePosition,
        Vec3& outGroundedPosition) const;

private:
    Vec3 _teleportDestination = Vec3::Zero;

    Vec3 _landingDirection = Vec3::Forward;
    bool _isPrepared = false;

public:
    static Shared<PlayerState_Replacement> Create();
};

NS_END
