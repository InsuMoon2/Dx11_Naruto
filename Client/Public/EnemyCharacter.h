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

    // Applies replicated S_Move/S_AddObject data to a remote enemy instance.
    void    Sync(const Protocol::ObjectInfo& info);

protected:
    HRESULT Bind_ShaderResources() override;

    // Builds the replicated enemy snapshot that the authority client relays to the server.
    Protocol::ObjectInfo Build_NetworkInfo();

    // Compares the next snapshot with the last sent snapshot to skip redundant sends.
    bool    Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const;

    // Sends the current enemy snapshot from the authority client to the server.
    void    Send_MovePacket(bool forceSend);

    // Captures BT blackboard and animation data into ObjectInfo for replication.
    void    Capture_NetworkAnimState(Protocol::ObjectInfo& outInfo);

    // Returns the object type this enemy writes into replicated packets.
    virtual Protocol::OBJECT_TYPE Get_EnemyObjectType() const = 0;

    // Maps a local animation state name to the replicated object state enum.
    virtual Protocol::OBJECT_STATE_TYPE To_EnemyObjectState(const string& animStateName) const = 0;

protected:
    Shared<Model>                   _model; // Shared enemy render and animation model.
    Shared<CombatStat>              _combatStat; // Shared enemy stat component.
    Shared<MovementComponent>       _movement; // Shared enemy movement component.
    Shared<AIController>            _aiController; // Shared BT controller for enemies.
    Shared<BehaviorTree>            _behavior; // Shared enemy behavior tree.
    Shared<AnimationStateComponent> _animState; // Shared enemy animation state component.
    Shared<Collider>                _collider; // Shared enemy collider handle.

    bool _networkDriven = false; // True when this enemy follows replicated server state.
    uint64 _networkObjectId = 0; // Server-assigned object id for this enemy.
    Vec3 _targetPos = Vec3::Zero; // Target position used by remote interpolation.
    float _targetRotY = 0.f; // Target yaw used by remote interpolation.
    float _lerpSpeed = 10.f; // Interpolation speed for replicated movement.
    float _snapDistanceSq = 25.f; // Snap instead of lerp when the delta is too large.
    bool _hasReceivedFirstSync = false; // Tracks whether the first replicated state arrived.

    float _syncTimer = 0.f; // Accumulates time until the next authority send.
    float _syncInterval = 0.05f; // Authority send interval for enemy state.

    Vec3 _lastSyncPos = Vec3::Zero; // Last sent position.
    float _lastSyncRotY = 0.f; // Last sent yaw.
    Protocol::OBJECT_STATE_TYPE _lastObjectState = Protocol::OBJECT_STATE_TYPE_IDLE; // Last sent object state.
    Protocol::MOVE_INPUT_DIR_TYPE _lastMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD; // Last sent move direction.
    Protocol::ANIM_PHASE_TYPE _lastAnimPhase = Protocol::ANIM_PHASE_START; // Last sent animation phase.
    Protocol::ATTACK_PROFILE_TYPE _lastAttackProfile = Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND; // Last sent attack profile.
    int32 _lastAttackComboIndex = 0; // Last sent combo index.

    string _hitEffectAssetName = "HitParticle"; // Effect tag used when the enemy is damaged.
    float _hitEffectHeightOffset = 1.f; // Y offset used for the damage effect.
    string _idleStateName = "Idle"; // Default idle state name for begin play and fallback sync.
};

NS_END
