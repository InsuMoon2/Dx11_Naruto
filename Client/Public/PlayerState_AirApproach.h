#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class WireMeshEffect;

class PlayerState_AirApproach : public IPlayerState
{
public:
    // 점프 이동 중에 어떤 결과를 내야할지
    enum class EArriveAction
    {
        None,
        WallAttach,
        GroundLand,
        ChangeState,
        END
    };

    struct FApproachDesc
    {
        Vec3 targetPosition = Vec3::Zero;
        float stopDistance = 0.25f;  // 목표 도달 판정 거리
        float moveSpeed = 33.f;
        float maxApproachTime = 1.f; // 무한 접근 방지

        Vec3 wallNormal = Vec3::Zero;

        bool gravityOff = true;  // 중력 끌지?

        EPlayerState animState = EPlayerState::JumpDash;        // 사용할 애니메이션
        EArriveAction arriveAction = EArriveAction::None;       // 도착 시 수행할 동작
        EPlayerState nextStateOnArrive = EPlayerState::END;     // ChangeState일 때 도착 후 전환할 상태 -> 추후에 공중공격으로 세팅
        EPlayerState nextStateOnFail = EPlayerState::JumpFall;  // 실패 시 전환할 상태 -> 기본 JumpFall

        Weak<WireMeshEffect> wireMeshEffect;
    };

public:
    PlayerState_AirApproach();
    ~PlayerState_AirApproach() override;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::AirApproach; }

    void Set_AirApproachDesc(const FApproachDesc& desc) { _approachDesc = desc; }

private:
    void Destroy_WireMesh();

private:
    FApproachDesc _approachDesc;
    float _elapsedTime = 0.f;
    bool _arrived = false;

public:
    static Shared<PlayerState_AirApproach> Create();
};

NS_END
