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

    bool _networkDriven = false;
    uint64 _networkObjectId = 0;
    Vec3 _targetPos = Vec3::Zero;
    float _targetRotY = 0.f; 
    float _lerpSpeed = 10.f; 
    float _snapDistanceSq = 25.f;
    bool _hasReceivedFirstSync = false; 

    float _syncTimer = 0.f;
    float _syncInterval = 0.05f; 

    Vec3 _lastSyncPos = Vec3::Zero; 
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

public:
    void Free() override;
};

NS_END
