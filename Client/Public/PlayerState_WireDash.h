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
    Vec3 _wireAttachPosition = Vec3::Zero;

    Weak<WireMeshEffect> _wireMeshEffect;

private:
    void Spawn_WireMesh(PlayerStateMachine* state, const Vec3& targetPosition);
    Weak<WireMeshEffect> Consume_WireMeshEffect();
    void Destroy_WireMesh();

public:
    static Shared<PlayerState_WireDash> Create();
};

NS_END
