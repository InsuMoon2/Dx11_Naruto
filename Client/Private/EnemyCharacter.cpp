#include "pch.h"
#include "EnemyCharacter.h"

#include "CombatStat.h"
#include "MovementComponent.h"
#include "AIController.h"
#include "BehaviorTree.h"
#include "Shader.h"
#include "Model.h"
#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "Collider.h"
#include "SkillObject.h"
#include "Client_PacketHandler.h"
#include "NetworkManager.h"
#include "AttachedEffectObject.h"

NS_BEGIN(Client)

// 몬스터/보스가 피격 반응으로 주목할 수 있는 전투 대상인지 검사한다.
// 플레이어/리모트 플레이어만 허용하고, 이미 파괴됐거나 죽은 객체는 제외한다.
static bool Is_ValidCombatTargetObject(const Shared<GameObject>& obj)
{
    if (!obj || obj->Is_Destroy())
        return false;

    const auto objectType = obj->Get_ObjectType();
    if (objectType != Protocol::OBJECT_TYPE_PLAYER &&
        objectType != Protocol::OBJECT_TYPE_REMOTE_PLAYER)
    {
        return false;
    }

    auto combatStat = obj->Get_Component<CombatStat>();
    if (combatStat && combatStat->Is_Dead())
        return false;

    return true;
}

// 현재 적과 후보 타겟 사이의 거리 제곱값을 구한다.
// 타겟이 없거나 Transform을 찾지 못하면 매우 큰 값으로 처리해서 비교에서 밀리게 만든다.
static float Get_TargetDistanceSq(const Shared<Transform>& selfTransform, const Shared<GameObject>& target)
{
    if (!selfTransform || !target)
        return FLT_MAX;

    auto targetTransform = target->Get_Transform();
    if (!targetTransform)
        return FLT_MAX;

    return Vec3::DistanceSquared(
        selfTransform->Get_WorldPosition(),
        targetTransform->Get_WorldPosition());
}

EnemyCharacter::EnemyCharacter(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Character(device, context)
{
}

EnemyCharacter::EnemyCharacter(const EnemyCharacter& rhs)
    : Character(rhs)
    , _lerpSpeed(rhs._lerpSpeed)
    , _snapDistanceSq(rhs._snapDistanceSq)
    , _syncInterval(rhs._syncInterval)
    , _hitEffectAssetName(rhs._hitEffectAssetName)
    , _hitEffectHeightOffset(rhs._hitEffectHeightOffset)
    , _idleStateName(rhs._idleStateName)
    , _retargetCooldownRemaining(rhs._retargetCooldownRemaining)
    , _retargetCooldown(rhs._retargetCooldown)
    , _retargetMinDamage(rhs._retargetMinDamage)
    , _retargetForceDamage(rhs._retargetForceDamage)
    , _retargetPreferCloserDistance(rhs._retargetPreferCloserDistance)
{
}

HRESULT EnemyCharacter::Initialize_Prototype()
{
    CHECK_FAILED(Character::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT EnemyCharacter::Initialize(void* arg)
{
    CHECK_FAILED(Character::Initialize(arg), E_FAIL);

    _networkDriven = false;
    _networkObjectId = 0;
    _targetPos = Vec3::Zero;
    _targetRotY = 0.f;
    _hasReceivedFirstSync = false;
    _syncTimer = 0.f;
    _lastSyncPos = Vec3::Zero;
    _lastSyncRotY = 0.f;
    _lastObjectState = Protocol::OBJECT_STATE_TYPE_IDLE;
    _lastMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;
    _lastAnimPhase = Protocol::ANIM_PHASE_START;
    _lastAttackProfile = Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND;
    _lastAttackComboIndex = 0;
    _lastAnimStateKey.clear();
    _lastHitReactionType = Protocol::HIT_REACTION_TYPE_DEFAULT;
    _lastHitReactionSerial = 0;
    _retargetCooldownRemaining = 0.f;

    return S_OK;
}

void EnemyCharacter::BeginPlay()
{
    Character::BeginPlay();

    if (_animState)
    {
        _animState->Play_State(_idleStateName);

        if (_model)
            _model->Play_Animation(0.f);
    }
}

void EnemyCharacter::Priority_Update(float timeDelta)
{
    Character::Priority_Update(timeDelta);
}

void EnemyCharacter::Update(float timeDelta)
{
    Character::Update(timeDelta);

    _retargetCooldownRemaining = max(0.f, _retargetCooldownRemaining - timeDelta);

    const bool isDead = (_combatStat && _combatStat->Is_Dead());
    if (isDead)
    {
        // 사망 후에는 투명 상태로 남아도 AI가 계속 공격을 스폰하지 않도록 즉시 중단한다.
        if (_model)
            _model->Play_Animation(timeDelta);
        return;
    }

    if (_networkDriven)
    {
        const Vec3 currentPos = _transformCom->Get_WorldPosition();
        const Vec3 newPos = Vec3::Lerp(currentPos, _targetPos, _lerpSpeed * timeDelta);
        _transformCom->Set_WorldPosition(newPos);

        const Quat currentRot = _transformCom->Get_LocalRotation();
        const Quat targetRot = Quat::CreateFromYawPitchRoll(_targetRotY, 0.f, 0.f);
        const Quat newRot = Quat::Slerp(currentRot, targetRot, _lerpSpeed * timeDelta);
        _transformCom->Set_LocalRotation(newRot);

        if (_animState)
            _animState->Apply_NetworkState();
    }
    else if (_aiController)
    {
        if (!GAME->Is_CinematicPlaying() && !GAME->Is_CinematicTransition())
        {
            _aiController->Update(timeDelta);
        }
    }

    if (_model)
        _model->Play_Animation(timeDelta);

    if (!_networkDriven && _networkObjectId != 0)
    {
        _syncTimer += timeDelta;
        if (_syncTimer >= _syncInterval)
        {
            _syncTimer = 0.f;
            Send_MovePacket(false);
        }
    }

}

void EnemyCharacter::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    const bool isDead = (_combatStat && _combatStat->Is_Dead());

    if (_collider && !isDead)
    {
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
        GAME->Add_Collider(_collider);
    }

    if (!isDead)
    {
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
        GAME->Add_RenderGroup(ERenderGroup::ShadowDynamic, GetSharedPtr());
    }
}

HRESULT EnemyCharacter::Render()
{
    if (!_model || !_shaderCom || Is_Destroy())
        return S_OK;

    CHECK_FAILED(Character::Render(), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();
    if (numMeshes == 0)
        return S_OK;

    if (FAILED(_model->Bind_BoneMatrices(_shaderCom, "g_BoneMatrices")))
        return S_OK;

    const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);
    const float outlineThickness = 0.0035f;

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);

    CHECK_FAILED(Bind_HitColor_ShaderParams(_shaderCom), E_FAIL);

    for (size_t i = 0; i < numMeshes; ++i)
    {
        _model->Bind_Material(_shaderCom, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

void EnemyCharacter::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    Character::OnBeginOverlap(self, other);
}

void EnemyCharacter::TakeDamage(const FDamageEvent& damageEvent)
{
    Character::TakeDamage(damageEvent);

    if (_combatStat && _combatStat->Is_Dead())
        return;

    if (_combatStat)
    {
        const bool wasAlive = !_combatStat->Is_Dead();

        _combatStat->Take_Damage(damageEvent);

        if (wasAlive && _combatStat->Is_Dead())
            OnDead(damageEvent);
    }
}

void EnemyCharacter::OnDamaged(const FDamageEvent& damageEvent)
{
    Character::OnDamaged(damageEvent);

    Set_RotationToDamageCauser(damageEvent);

    if (damageEvent.damage > 0.f)
    {
        // 몬스터/보스는 공격 종류와 상관없이 실제 데미지를 받으면 기본 HitParticle을 항상 재생한다.
        if (_transformCom && !_hitEffectAssetName.empty())
        {
            Vec3 hitEffectPosition = _transformCom->Get_WorldPosition();
            hitEffectPosition.y += _hitEffectHeightOffset;

            SkillObject::Spawn_Effect_Once(_hitEffectAssetName, hitEffectPosition);
        }
    }

    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard)
        {
            if (Is_SuperArmorActiveFromSkillState())
            {
                if (damageEvent.damageCauser && !damageEvent.damageCauser->Is_Destroy())
                {
                    blackboard->Set_ValueAsObject("LastDamageCauser", damageEvent.damageCauser);

                    if (Should_RetargetToDamageCauser(damageEvent))
                        Apply_DamageCauserTarget(damageEvent);
                }

                return;
            }

            static int32 sHitReactionSerial = 0;
            ++sHitReactionSerial;

            string hitAnimState = damageEvent.hitAnimStateOverride;
            if (hitAnimState.empty())
            {
                hitAnimState = "Hit";
                switch (damageEvent.hitReactionType)
                {
                case EHitReactionType::Launch:
                    hitAnimState = "Hit_Launch";
                    break;

                case EHitReactionType::BlowOff:
                    hitAnimState = "Hit_BlowOff";
                    break;

                case EHitReactionType::Down:
                    hitAnimState = "Hit_Down";
                    break;

                case EHitReactionType::Air:
                    hitAnimState = "Hit_Air";
                    break;

                case EHitReactionType::Air_Down:
                    hitAnimState = "Hit_Air_Down";
                    break;

                case EHitReactionType::Stagger:
                case EHitReactionType::Default:
                default:
                    hitAnimState = "Hit";
                    break;
                }
            }

            blackboard->Set_ValueAsBool("IsHit", true);
            blackboard->Set_ValueAsInt("HitReactionType", static_cast<int32>(damageEvent.hitReactionType));
            blackboard->Set_ValueAsInt("HitReactionSerial", sHitReactionSerial);
            blackboard->Set_ValueAsString("HitAnimState", hitAnimState);

            if (damageEvent.damageCauser && !damageEvent.damageCauser->Is_Destroy())
            {
                blackboard->Set_ValueAsObject("LastDamageCauser", damageEvent.damageCauser);

                if (Should_RetargetToDamageCauser(damageEvent))
                    Apply_DamageCauserTarget(damageEvent);
            }
        }
    }
}

bool EnemyCharacter::Is_SuperArmorActiveFromSkillState() const
{
    const auto objectType = Get_ObjectType();
    if (objectType != Protocol::OBJECT_TYPE_MONSTER_WOOD &&
        objectType != Protocol::OBJECT_TYPE_MONSTER_LEAF &&
        objectType != Protocol::OBJECT_TYPE_BOSS_PAIN)
    {
        return false;
    }

    string currentAnimStateName = "";

    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard && blackboard->HasKey("AnimState"))
            currentAnimStateName = blackboard->Get_ValueAsString("AnimState");
    }

    if (currentAnimStateName.empty() && _animState)
        currentAnimStateName = _animState->Get_CurrentStateName();

    return currentAnimStateName.rfind("Skill_", 0) == 0;
}

bool EnemyCharacter::Should_RetargetToDamageCauser(const FDamageEvent& damageEvent) const
{
    if (!_behavior || !_transformCom)
        return false;

    auto blackboard = _behavior->Get_Blackboard();
    if (!blackboard)
        return false;

    auto damageCauser = damageEvent.damageCauser;
    if (!Is_ValidCombatTargetObject(damageCauser))
        return false;

    auto currentTarget = blackboard->Get_ValueAsObject("TargetObjectKey");
    if (!Is_ValidCombatTargetObject(currentTarget))
        return true;

    if (currentTarget == damageCauser)
        return true;

    if (damageEvent.damage >= _retargetForceDamage)
        return true;

    if (_retargetCooldownRemaining > 0.f)
        return false;

    if (damageEvent.damage < _retargetMinDamage)
        return false;

    const float currentTargetDistanceSq = Get_TargetDistanceSq(_transformCom, currentTarget);
    const float damageCauserDistanceSq = Get_TargetDistanceSq(_transformCom, damageCauser);
    const float preferCloserDistanceSq = _retargetPreferCloserDistance * _retargetPreferCloserDistance;

    return damageCauserDistanceSq + preferCloserDistanceSq < currentTargetDistanceSq;
}

void EnemyCharacter::Apply_DamageCauserTarget(const FDamageEvent& damageEvent)
{
    if (!_behavior)
        return;

    auto blackboard = _behavior->Get_Blackboard();
    if (!blackboard)
        return;

    auto damageCauser = damageEvent.damageCauser;
    if (!Is_ValidCombatTargetObject(damageCauser))
        return;

    blackboard->Set_ValueAsObject("TargetObjectKey", damageCauser);
    _retargetCooldownRemaining = _retargetCooldown;

    auto causerTransform = damageCauser->Get_Transform();
    if (!causerTransform)
        return;

    blackboard->Set_ValueAsVector(
        "TargetLocationKey",
        causerTransform->Get_WorldPosition());
}

void EnemyCharacter::OnDead(const FDamageEvent& damageEvent)
{
    Character::OnDead(damageEvent);

    if (_collider)
        _collider->Set_IsActive(false);

    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard)
            blackboard->Set_ValueAsBool("IsDead", true);
    }

    // 사망 시 몬스터 몸이 안 보이게 되므로 그 자리에 Test_Smoke 이펙트를 뿌림
    if (_transformCom)
    {
        Vec3 center = _transformCom->Get_WorldPosition() + Vec3(0.f, 1.f, 0.f);

        auto SpawnSmoke = [](const Vec3& pos)
        {
            AttachedEffectObject::FAttachedEffectObjectDesc desc{};
            desc.effectAssetName = "Test_Smoke";
            desc.loopOverride = false;
            
            auto effect = GAME->Clone_And_Add_GameObject(
                ETOI(ELevelType::Static),
                Protocol::OBJECT_TYPE_ATTACHED_EFFECT,
                GAME->Current_Level(),
                TEXT("Layer_Effect"),
                &desc);

            if (effect && effect->Get_Transform())
            {
                effect->Get_Transform()->Set_WorldPosition(pos);
                effect->Get_Transform()->Set_LocalScale(Vec3(0.5f, 0.5f, 0.5f)); // 약간 크게
            }
        };

        // 중심에 하나
        SpawnSmoke(center);

        // 주변에 4개 (지폭천성과 비슷한 느낌으로)
        int32 burstCount = 4;
        float radius = 1.2f;
        for (int32 i = 0; i < burstCount; ++i)
        {
            float angle = XM_2PI * (static_cast<float>(i) / static_cast<float>(burstCount));
            Vec3 offset = Vec3(cosf(angle), 0.f, sinf(angle)) * radius;
            SpawnSmoke(center + offset);
        }
    }
}

void EnemyCharacter::Sync(const Protocol::ObjectInfo& info)
{
    const Vec3 nextPos(info.pos().x(), info.pos().y(), info.pos().z());
    const float nextRotY = info.rot_y();

    const Vec3 currentPos = _transformCom->Get_WorldPosition();
    const float distSq = Vec3::DistanceSquared(currentPos, nextPos);

    if (!_hasReceivedFirstSync || distSq >= _snapDistanceSq)
    {
        _transformCom->Set_WorldPosition(nextPos);
        _transformCom->Set_LocalRotation(Quat::CreateFromYawPitchRoll(nextRotY, 0.f, 0.f));

        _targetPos = nextPos;
        _targetRotY = nextRotY;
        _hasReceivedFirstSync = true;
    }
    else
    {
        _targetPos = nextPos;
        _targetRotY = nextRotY;
    }

    if (_animState)
        _animState->Read_FromObjectInfo(info);

    if (_combatStat && info.has_stat())
    {
        Protocol::CombatStat stat = info.stat();
        _combatStat->Sync_FromProtobuf(stat);
    }
}

void EnemyCharacter::Set_RotationToDamageCauser(const FDamageEvent& damageEvent)
{
    if (!_transformCom)
        return;

    auto damageCauser = damageEvent.damageCauser;
    if (!damageCauser)
        return;

    auto causerTransform = damageCauser->Get_Component<Transform>();
    if (!causerTransform)
        return;

    const Vec3 myPos = _transformCom->Get_WorldPosition();
    Vec3 targetPos = causerTransform->Get_WorldPosition();

    targetPos.y = myPos.y;

    Vec3 lookDir = targetPos - myPos;
    if (lookDir.LengthSquared() <= 0.0001f)
        return;

    lookDir.Normalize();

    _transformCom->LookAt(myPos + lookDir);
}

HRESULT EnemyCharacter::Render_Shadow()
{
    if (!_model || !_shaderCom || Is_Destroy())
    {
#ifdef _DEBUG
        LOG_WARN("[EnemyShadow] skipped. model={}, shader={}, destroyed={}, name='{}', guid='{}'",
            _model != nullptr,
            _shaderCom != nullptr,
            Is_Destroy(),
            Utils::ToString(Get_Name()),
            Get_GUID());
#endif
        return S_FALSE;
    }

    CHECK_FAILED(Bind_ShadowShaderResources(), E_FAIL);
    CHECK_FAILED(_model->Bind_BoneMatrices(_shaderCom, "g_BoneMatrices"), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();
    if (numMeshes == 0)
    {
#ifdef _DEBUG
        LOG_WARN("[EnemyShadow] skipped. numMeshes=0, name='{}', guid='{}'",
            Utils::ToString(Get_Name()),
            Get_GUID());
#endif
        return S_FALSE;
    }

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);
        CHECK_FAILED(_shaderCom->Begin_Pass(2), E_FAIL);
        CHECK_FAILED(_model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT EnemyCharacter::Bind_ShadowShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(GAME->Bind_ShadowMatrices(_shaderCom, "g_ViewMatrix", "g_ProjMatrix"), E_FAIL);

    return S_OK;

}

HRESULT EnemyCharacter::Bind_ShaderResources()
{
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

Protocol::ObjectInfo EnemyCharacter::Build_NetworkInfo()
{
    Protocol::ObjectInfo info{};
    info.set_objectid(_networkObjectId);
    info.set_objecttype(Get_EnemyObjectType());

    const Vec3 pos = _transformCom->Get_WorldPosition();
    auto* protoPos = info.mutable_pos();
    protoPos->set_x(pos.x);
    protoPos->set_y(pos.y);
    protoPos->set_z(pos.z);

    const float rotY = _transformCom->Get_LocalRotation().ToEuler().y;
    info.set_rot_y(rotY);

    Capture_NetworkAnimState(info);

    if (_combatStat)
        _combatStat->Serialize_ToProtobuf(*info.mutable_stat());

    return info;
}

bool EnemyCharacter::Should_SendMovePacket(const Protocol::ObjectInfo& nextInfo) const
{
    const Vec3 nextPos(
        nextInfo.pos().x(),
        nextInfo.pos().y(),
        nextInfo.pos().z());

    const float posDeltaSq = Vec3::DistanceSquared(nextPos, _lastSyncPos);
    const float rotDelta = fabsf(nextInfo.rot_y() - _lastSyncRotY);

    if (posDeltaSq > 0.0001f)
        return true;

    if (rotDelta > XMConvertToRadians(1.f))
        return true;

    if (nextInfo.object_state() != _lastObjectState)
        return true;

    if (nextInfo.move_dir() != _lastMoveDir)
        return true;

    if (nextInfo.anim_phase() != _lastAnimPhase)
        return true;

    if (nextInfo.anim_force_restart())
        return true;

    if (nextInfo.attack_profile() != _lastAttackProfile)
        return true;

    if (nextInfo.attack_combo_index() != _lastAttackComboIndex)
        return true;

    if (nextInfo.anim_state_key() != _lastAnimStateKey)
        return true;

    if (nextInfo.hit_reaction_type() != _lastHitReactionType)
        return true;

    if (nextInfo.hit_reaction_serial() != _lastHitReactionSerial)
        return true;

    return false;
}

void EnemyCharacter::Send_MovePacket(bool forceSend)
{
    Protocol::ObjectInfo info = Build_NetworkInfo();

    if (!forceSend && !Should_SendMovePacket(info))
        return;

    auto buf = Client_PacketHandler::Make_C_Move(info);
    if (!buf)
        return;

    GET_SINGLE(NetworkManager)->Send_Packet(buf);

    _lastSyncPos = Vec3(info.pos().x(), info.pos().y(), info.pos().z());
    _lastSyncRotY = info.rot_y();
    _lastObjectState = info.object_state();
    _lastMoveDir = info.move_dir();
    _lastAnimPhase = info.anim_phase();
    _lastAttackProfile = info.attack_profile();
    _lastAttackComboIndex = info.attack_combo_index();
    _lastAnimStateKey = info.anim_state_key();
    _lastHitReactionType = info.hit_reaction_type();
    _lastHitReactionSerial = info.hit_reaction_serial();
}

void EnemyCharacter::Capture_NetworkAnimState(Protocol::ObjectInfo& outInfo)
{
    AnimationStateComponent::FAnimReplicatedState replicatedState{};
    replicatedState.state = Protocol::OBJECT_STATE_TYPE_IDLE;
    replicatedState.dir = EMoveInputDirection::Forward;
    replicatedState.phase = _animState ? _animState->Get_CurrentAnimPhase() : EAnimPhase::Start;
    replicatedState.forceRestart = false;
    replicatedState.attackProfile = EAttackProfileType::Hand_Ground;
    replicatedState.attackComboIndex = 0;
    replicatedState.animStateKey = "";
    replicatedState.hitReactionType = EHitReactionType::Default;
    replicatedState.hitReactionSerial = 0;

    string animStateName = _idleStateName;

    // Convert the BT-selected animation key into the replicated network state.
    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard)
        {
            if (blackboard->HasKey("AnimState"))
            {
                const string nextAnimState = blackboard->Get_ValueAsString("AnimState");
                if (!nextAnimState.empty())
                    animStateName = nextAnimState;
            }

            if (blackboard->HasKey("AnimDirection"))
            {
                replicatedState.dir = static_cast<EMoveInputDirection>(
                    blackboard->Get_ValueAsInt("AnimDirection"));
            }
        }
    }

    replicatedState.state = To_EnemyObjectState(animStateName);
    replicatedState.animStateKey = animStateName;

    Protocol::MOVE_INPUT_DIR_TYPE nextMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;
    switch (replicatedState.dir)
    {
    case EMoveInputDirection::Backward:
        nextMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_BACKWARD;
        break;
    case EMoveInputDirection::Left:
        nextMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_LEFT;
        break;
    case EMoveInputDirection::Right:
        nextMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_RIGHT;
        break;
    default:
        nextMoveDir = Protocol::MOVE_INPUT_DIR_TYPE_FORWARD;
        break;
    }

    replicatedState.forceRestart =
        (_lastObjectState != replicatedState.state) ||
        (_lastMoveDir != nextMoveDir) ||
        (_lastAnimPhase != Protocol::ANIM_PHASE_START && replicatedState.phase == EAnimPhase::Start);

    if (_animState)
    {
        _animState->Sync_FromNetwork(replicatedState);
        _animState->Write_ToObjectInfo(outInfo);
    }
    else
    {
        outInfo.set_object_state(replicatedState.state);
        outInfo.set_move_dir(nextMoveDir);
        outInfo.set_anim_phase(Protocol::ANIM_PHASE_START);
        outInfo.set_anim_force_restart(replicatedState.forceRestart);
        outInfo.set_attack_profile(Protocol::ATTACK_PROFILE_TYPE_HAND_GROUND);
        outInfo.set_attack_combo_index(0);
        outInfo.set_anim_state_key(replicatedState.animStateKey);
        outInfo.set_hit_reaction_type(Protocol::HIT_REACTION_TYPE_DEFAULT);
        outInfo.set_hit_reaction_serial(0);
    }
}

void EnemyCharacter::Free()
{
    Character::Free();
}

NS_END
