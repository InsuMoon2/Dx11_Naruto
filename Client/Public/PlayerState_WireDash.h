#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class WireMeshEffect;

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

public:
    bool Build_ViewportCenterRay(Vec3& outStart, Vec3& outDir) const;

private:
    bool _isWireAttach = false;
    bool _wireAttachGround = false; // 현재 와이어 대쉬가 벽이 아니라 바닥 착지용 접근인지 Update에서 조기 전환 판단할 때 사용한다.
    float _wireTargetDistance = 0.f; // 와이어 표면까지의 초기 거리를 저장해 가까운 바닥 목표에서 AirApproach를 더 빨리 시작할지 결정할 때 사용한다.
    Vec3 _wireAttachPosition = Vec3::Zero;

    Weak<WireMeshEffect> _wireMeshEffect;

private:
    bool Should_BeginAirApproachEarly(PlayerStateMachine* state) const; // 가까운 바닥 목표일 때 WireDash 풀 애니를 다 기다리지 않고 접근을 시작해 질질 끄는 느낌을 줄인다.
    void Spawn_WireMesh(PlayerStateMachine* state, const Vec3& targetPosition);
    Weak<WireMeshEffect> Consume_WireMeshEffect();
    void Destroy_WireMesh();

public:
    static Shared<PlayerState_WireDash> Create();
};

NS_END
