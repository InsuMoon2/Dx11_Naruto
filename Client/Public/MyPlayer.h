#pragma once

#include "Player.h"

NS_BEGIN(Client)

class CombatStat;
class PlayerStateMachine;
class PlayerController;
class InputComponent;
class MovementComponent;
class SkillComponent;
class AnimationStateComponent;

class MyPlayer final : public Player
{
    GENERATED_BODY(MyPlayer)

public:
    explicit MyPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit MyPlayer(const MyPlayer& rhs);
    virtual ~MyPlayer() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    // 내 캐릭터는 서버 위치 무시
    void Sync(const Protocol::ObjectInfo& info) override {}

public:
    void Force_SendMovePacket();

private:
    void    Send_MovePacket(bool forceSend);
    Protocol::ObjectInfo Build_NetworkInfo() const;
    bool    Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const;

    HRESULT Ready_Components() override;

private:
    Shared<InputComponent>      _input;
    Shared<MovementComponent>   _movement;
    Shared<PlayerController>    _playerController;
    Shared<PlayerStateMachine>  _stateMachine;
    Shared<SkillComponent>      _skill;

private:
    float _syncTimer = 0.f;
    float _syncInterval = 1.f / 30.f;

    Vec3 _lastSyncPos = {};
    float _lastSyncRotY = 0.f;
    Protocol::OBJECT_STATE_TYPE   _lastObjectState = Protocol::OBJECT_STATE_TYPE_IDLE;
    Protocol::MOVE_INPUT_DIR_TYPE _lastMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;

    Protocol::ANIM_PHASE_TYPE     _lastAnimPhase = Protocol::ANIM_PHASE_START;

public:
    static shared_ptr<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
