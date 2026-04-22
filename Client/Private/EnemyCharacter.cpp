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

NS_BEGIN(Client)

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
        _aiController->Update(timeDelta);
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

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
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

    if (damageEvent.damage > 0.f && _transformCom && !_hitEffectAssetName.empty())
    {
        Vec3 hitEffectPosition = _transformCom->Get_WorldPosition();
        hitEffectPosition.y += _hitEffectHeightOffset;

        SkillObject::Spawn_Effect_Once(_hitEffectAssetName, hitEffectPosition);
    }

    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard)
        {
            static int32 sHitReactionSerial = 0;
            ++sHitReactionSerial;

            string hitAnimState = "Hit";
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

            blackboard->Set_ValueAsBool("IsHit", true);
            blackboard->Set_ValueAsInt("HitReactionType", static_cast<int32>(damageEvent.hitReactionType));
            blackboard->Set_ValueAsInt("HitReactionSerial", sHitReactionSerial);
            blackboard->Set_ValueAsString("HitAnimState", hitAnimState);
        }
    }
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
