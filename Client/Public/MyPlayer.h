#pragma once

#include "Player.h"

NS_BEGIN(Engine)
class MovementComponent;
NS_END

NS_BEGIN(Client)

class CombatStat;
class PlayerStateMachine;
class PlayerController;
class InputComponent;
class AnimationStateComponent;
class TargetComponent;

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
    void    BeginPlay() override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    // 내 캐릭터는 서버 위치 무시
    void    Sync(const Protocol::ObjectInfo& info) override {}
    void    Force_SendMovePacket();

public:
    void    Enable_Hitbox(EHitboxTarget target);
    void    Disable_Hitbox(EHitboxTarget target);

    void    Disable_All_Hitboxes();

public:
    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

    void    Add_ComboHit();

private:
    void    Update_Combo(float timeDelta);
    void    Update_Targetting(float timeDelta);

private:
    // 연속 콤보 히트 카운트 — DelegateHub::OnPlayerComboHit에 전달
    uint32 _comboHitCount = 0;

    // 콤보 유지 타이머 — 일정 시간 히트 없으면 리셋
    float _comboDecayTimer = 0.f;
    static constexpr float COMBO_DECAY_TIME = 3.f;

private:
    void    Send_MovePacket(bool forceSend);
    Protocol::ObjectInfo Build_NetworkInfo() const;
    bool    Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const;

    HRESULT Ready_Components() override;
    HRESULT Ready_HitboxColliders();

private:
    Shared<InputComponent>      _input;
    Shared<MovementComponent>   _movement;
    Shared<PlayerController>    _playerController;
    Shared<PlayerStateMachine>  _stateMachine;

    Shared<TargetComponent>     _target;
    Shared<Collider>            _targetCollider;

    // 몸통 충돌체
    Shared<Collider>            _collider;

    // 격투형 충돌체
    array<Shared<Collider>, ETOI(EHitboxTarget::END)> _hitboxColliders;
    // 충돌체 붙일 뼈 세팅
    array<string, ETOI(EHitboxTarget::END)> _hitboxBoneNames;

private:
    float _syncTimer = 0.f;
    float _syncInterval = 1.f / 30.f;

    Vec3 _lastSyncPos = {};
    Vec3 _lastSyncRot = {};
    Protocol::OBJECT_STATE_TYPE   _lastObjectState = Protocol::OBJECT_STATE_TYPE_IDLE;
    Protocol::MOVE_INPUT_DIR_TYPE _lastMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;

    Protocol::ANIM_PHASE_TYPE     _lastAnimPhase = Protocol::ANIM_PHASE_START;

    Protocol::ATTACK_PROFILE_TYPE _lastAttackProfile = Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND;
    int32 _lastAttackComboIndex = 0;

    // 마지막으로 송신한 실제 애니메이션 상태 키다. 같은 OBJECT_STATE 내 세부 상태 변화 감지에 사용한다.
    string _lastAnimStateKey = "";

    // 마지막으로 송신한 피격 타입이다. 원격 히트 분기 변화 감지에 사용한다.
    Protocol::HIT_REACTION_TYPE _lastHitReactionType = Protocol::HIT_REACTION_TYPE_DEFAULT;

    // 마지막으로 송신한 피격 serial이다. 같은 Hit 재시작을 패킷으로 다시 보내기 위해 비교한다.
    uint32 _lastHitReactionSerial = 0;

public:
    static shared_ptr<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
