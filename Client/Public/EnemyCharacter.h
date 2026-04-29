#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class BehaviorTree;
class Model;
class Collider;
class MovementComponent;
NS_END

NS_BEGIN(Client)

class CombatStat;
class AIController;
class AnimationStateComponent;

class EnemyCharacter abstract : public Character
{
    GENERATED_BODY(EnemyCharacter)

public:
    explicit EnemyCharacter(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EnemyCharacter(const EnemyCharacter& rhs);
    virtual ~EnemyCharacter() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    TakeDamage(const FDamageEvent& damageEvent) override;
    void    OnDamaged(const FDamageEvent& damageEvent) override;
    void    OnDead(const FDamageEvent& damageEvent) override;

public:
    void    Set_NetworkDriven(bool enabled) { _networkDriven = enabled; }
    bool    Is_NetworkDriven() const { return _networkDriven; }
    void    Set_NetworkObjectId(uint64 objectId) { _networkObjectId = objectId; }

    void    Sync(const Protocol::ObjectInfo& info);

    void    Set_RotationToDamageCauser(const FDamageEvent& damageEvent);

    HRESULT Render_Shadow() override;

protected:
    HRESULT Bind_ShadowShaderResources();

    // 특정 몬스터/보스가 스킬 애니메이션을 재생 중일 때 피격 리액션을 무시할지 판단한다.
    // 현재는 Monster_Wood, Monster_Leaf, Boss_Pain만 Skill_ 접두사 상태에서 슈퍼아머를 사용한다.
    bool    Is_SuperArmorActiveFromSkillState() const;

    // 피격 시 들어온 공격자를 실제 전투 타겟으로 갈아탈지 판단한다.
    // 몬스터/보스 공통으로 호출되며, 타겟 전환이 너무 잦아지지 않도록 거리/쿨다운/데미지를 함께 본다.
    bool    Should_RetargetToDamageCauser(const FDamageEvent& damageEvent) const;

    // 피격한 공격자를 블랙보드 타겟으로 반영하고 위치 키까지 함께 갱신한다.
    // Should_RetargetToDamageCauser가 true일 때만 호출해서 자연스러운 어그로 전환을 만든다.
    void    Apply_DamageCauserTarget(const FDamageEvent& damageEvent);

protected:
    HRESULT Bind_ShaderResources() override;

    Protocol::ObjectInfo Build_NetworkInfo();

    bool    Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const;

    void    Send_MovePacket(bool forceSend);
    void    Capture_NetworkAnimState(Protocol::ObjectInfo& outInfo);

    virtual Protocol::OBJECT_TYPE Get_EnemyObjectType() const = 0;
    virtual Protocol::OBJECT_STATE_TYPE To_EnemyObjectState(const string& animStateName) const = 0;

protected:
    Shared<Model>                   _model; 
    Shared<CombatStat>              _combatStat; 
    Shared<MovementComponent>       _movement; 
    Shared<AIController>            _aiController; 
    Shared<BehaviorTree>            _behavior; 
    Shared<AnimationStateComponent> _animState;
    Shared<Collider>                _collider;

protected:
    bool    _networkDriven = false;
    uint64  _networkObjectId = 0;
    Vec3    _targetPos = Vec3::Zero;
    float   _targetRotY = 0.f; 
    float   _lerpSpeed = 10.f; 
    float   _snapDistanceSq = 25.f;
    bool    _hasReceivedFirstSync = false; 

    float   _syncTimer = 0.f;
    float   _syncInterval = 0.05f; 

    Vec3     _lastSyncPos = Vec3::Zero; 
    float _lastSyncRotY = 0.f; 
    Protocol::OBJECT_STATE_TYPE _lastObjectState = Protocol::OBJECT_STATE_TYPE_IDLE; 
    Protocol::MOVE_INPUT_DIR_TYPE _lastMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD; 
    Protocol::ANIM_PHASE_TYPE _lastAnimPhase = Protocol::ANIM_PHASE_START; 
    Protocol::ATTACK_PROFILE_TYPE _lastAttackProfile = Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND; 
    int32 _lastAttackComboIndex = 0;

    string _lastAnimStateKey = "";
    Protocol::HIT_REACTION_TYPE _lastHitReactionType = Protocol::HIT_REACTION_TYPE_DEFAULT;
    uint32 _lastHitReactionSerial = 0;

    string _hitEffectAssetName = "HitParticle"; 
    float _hitEffectHeightOffset = 1.f; 
    string _idleStateName = "Idle";

    float _retargetCooldownRemaining = 0.f; // 최근 타겟 전환 이후 다시 갈아타기까지 남은 대기 시간이다.
    float _retargetCooldown = 0.9f; // 몬스터/보스가 피격으로 타겟을 바꾸는 최소 간격이다.
    float _retargetMinDamage = 6.f; // 약한 스치기 공격으로는 타겟이 쉽게 바뀌지 않게 하는 최소 데미지 기준이다.
    float _retargetForceDamage = 16.f; // 이 값을 넘는 강한 피격은 현재 타겟보다 우선해서 즉시 주목하게 만든다.
    float _retargetPreferCloserDistance = 2.25f; // 새 공격자가 현재 타겟보다 이만큼 더 가까우면 자연스럽게 시선을 돌리게 하는 거리 여유치다.

public:
    void Free() override;
};

NS_END
