#include "pch.h"
#include "BTTask_ChibakuTensei.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "Character.h"
#include "GameObject.h"
#include "GameObject_Factory.h"
#include "MovementComponent.h"
#include "Skill_ChibakuTensei.h"
#include "Transform.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_ChibakuTensei)

bool BTTask_ChibakuTensei::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_ChibakuTensei";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Cooldown Remain Key", "cooldown_remain_key", _cooldownRemainKey);
    PROPERTY_STRING_JSON("Global Cooldown Remain Key", "global_cooldown_remain_key", _globalCooldownRemainKey);
    PROPERTY_STRING_JSON("Start Anim State", "start_anim_state", _startAnimState);
    PROPERTY_STRING_JSON("Attack Anim State", "attack_anim_state", _attackAnimState);
    PROPERTY_STRING_JSON("Hit Anim State Override", "hit_anim_state_override", _hitAnimStateOverride);

    PROPERTY_FLOAT_JSON("Cooldown", "cooldown", _cooldown, 0.f, 60.f);
    PROPERTY_FLOAT_JSON("Global Cooldown", "global_cooldown", _globalCooldown, 0.f, 30.f);
    PROPERTY_FLOAT_JSON("Approach Duration", "approach_duration", _approachDuration, 0.1f, 5.f);
    PROPERTY_FLOAT_JSON("Attack Start Range", "attack_start_range", _attackStartRange, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Hit Range", "hit_range", _hitRange, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Impact Time", "impact_time", _impactTime, 0.f, 5.f);
    PROPERTY_FLOAT_JSON("Attack Duration", "attack_duration", _attackDuration, 0.1f, 5.f);
    PROPERTY_BOOL_JSON("Use Sprint", "use_sprint", _useSprint);
    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    PROPERTY_FLOAT_JSON("Damage", "damage", _damage, 0.f, 999.f);
    PROPERTY_FLOAT_JSON("Launch Power", "launch_power", _launchPower, 0.f, 300.f);
    PROPERTY_FLOAT_JSON("Launch Up", "launch_up", _launchUp, -50.f, 80.f);

    return true;
}

BTTask_ChibakuTensei::BTTask_ChibakuTensei()
{
}

BTTask_ChibakuTensei::BTTask_ChibakuTensei(const BTTask_ChibakuTensei& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _cooldownRemainKey(rhs._cooldownRemainKey)
    , _globalCooldownRemainKey(rhs._globalCooldownRemainKey)
    , _startAnimState(rhs._startAnimState)
    , _attackAnimState(rhs._attackAnimState)
    , _hitAnimStateOverride(rhs._hitAnimStateOverride)
    , _cooldown(rhs._cooldown)
    , _globalCooldown(rhs._globalCooldown)
    , _approachDuration(rhs._approachDuration)
    , _attackStartRange(rhs._attackStartRange)
    , _hitRange(rhs._hitRange)
    , _impactTime(rhs._impactTime)
    , _attackDuration(rhs._attackDuration)
    , _useSprint(rhs._useSprint)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _damage(rhs._damage)
    , _launchPower(rhs._launchPower)
    , _launchUp(rhs._launchUp)
    , _phase(EChibakuTenseiTaskPhase::Approach)
    , _phaseElapsed(0.f)
    , _impactApplied(false)
    , _previousOrientRotationToMovement(false)
{
}

void BTTask_ChibakuTensei::Initialize()
{
    BTTask::Initialize();

    _phase = EChibakuTenseiTaskPhase::Approach;
    _phaseElapsed = 0.f;
    _impactApplied = false;
    _previousOrientRotationToMovement = false;
}

EBTNodeResult BTTask_ChibakuTensei::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    auto target = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!target || target->Is_Destroy())
    {
        Finish_Chibaku(blackboard, owner, false);
        return _lastResult = EBTNodeResult::Failed;
    }

    if (_phase == EChibakuTenseiTaskPhase::Approach && _phaseElapsed <= 0.f)
    {
        if (Is_CooldownBlocked(blackboard))
            return _lastResult = EBTNodeResult::Failed;

        Begin_Chibaku(blackboard, owner);
    }

    Tick_Phase(timeDelta);

    if (_faceTarget)
        Face_Target(owner, target);

    if (_phase == EChibakuTenseiTaskPhase::Approach)
    {
        const bool reachedTarget = Update_Approach(blackboard, owner, target);
        if (reachedTarget || _phaseElapsed >= _approachDuration)
            Begin_Attack(blackboard);

        return _lastResult = EBTNodeResult::InProgress;
    }

    if (_phase == EChibakuTenseiTaskPhase::Attack)
    {
        Update_AttackImpact(owner, target);

        auto animState = owner->Get_Component<AnimationStateComponent>();
        const bool attackTimeFinished = _phaseElapsed >= _attackDuration;
        const bool animFinished = !animState || animState->Is_CurrentStateFinished();

        if (!attackTimeFinished || !animFinished)
            return _lastResult = EBTNodeResult::InProgress;

        Finish_Chibaku(blackboard, owner, true);
        _phase = EChibakuTenseiTaskPhase::Finish;

        return _lastResult = EBTNodeResult::Succeeded;
    }

    Finish_Chibaku(blackboard, owner, false);
    return _lastResult = EBTNodeResult::Succeeded;
}

bool BTTask_ChibakuTensei::Is_CooldownBlocked(const Shared<Blackboard>& blackboard) const
{
    if (!blackboard)
        return true;

    const float cooldownRemain = !_cooldownRemainKey.empty() && blackboard->HasKey(_cooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_cooldownRemainKey)
        : 0.f;

    const float globalCooldownRemain = !_globalCooldownRemainKey.empty() && blackboard->HasKey(_globalCooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_globalCooldownRemainKey)
        : 0.f;

    return cooldownRemain > 0.f || globalCooldownRemain > 0.f;
}

void BTTask_ChibakuTensei::Tick_Phase(float timeDelta)
{
    _phaseElapsed += timeDelta;
}

void BTTask_ChibakuTensei::Begin_Chibaku(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner)
{
    auto movement = owner ? owner->Get_Component<MovementComponent>() : nullptr;
    if (movement)
    {
        _previousOrientRotationToMovement = movement->Get_OrientRotationToMovement();
        movement->Set_OrientRotationToMovement(false);
        movement->Resolve_CharacterBodyPenetration();
    }

    _phase = EChibakuTenseiTaskPhase::Approach;
    _phaseElapsed = 0.f;
    _impactApplied = false;

    Request_AnimState(blackboard, _startAnimState);
}

bool BTTask_ChibakuTensei::Update_Approach(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner, const Shared<GameObject>& target)
{
    auto ownerTransform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto targetTransform = target ? target->Get_Component<Transform>() : nullptr;
    auto movement = owner ? owner->Get_Component<MovementComponent>() : nullptr;

    if (!blackboard || !ownerTransform || !targetTransform)
        return false;

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 toTarget = targetPos - ownerPos;
    toTarget.y = 0.f;

    const float distance = toTarget.Length();
    if (distance <= _attackStartRange)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        if (movement)
            movement->Resolve_CharacterBodyPenetration();

        return true;
    }

    Vec3 moveDir = Utils::Safe_Normalize(toTarget, ownerTransform->Get_WorldForward());

    blackboard->Set_ValueAsFloat("MoveAxisX", moveDir.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", moveDir.z);
    blackboard->Set_ValueAsBool("Sprint", _useSprint);

    if (movement)
        movement->Resolve_CharacterBodyPenetration();

    return false;
}

void BTTask_ChibakuTensei::Begin_Attack(const Shared<Blackboard>& blackboard)
{
    if (blackboard)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
    }

    _phase = EChibakuTenseiTaskPhase::Attack;
    _phaseElapsed = 0.f;
    _impactApplied = false;

    Request_AnimState(blackboard, _attackAnimState);

    if (blackboard && _requestAnimEnd)
        blackboard->Set_ValueAsBool("AnimRequestEnd", true);
}

void BTTask_ChibakuTensei::Update_AttackImpact(const Shared<GameObject>& owner, const Shared<GameObject>& target)
{
    if (_impactApplied)
        return;

    if (_phaseElapsed < _impactTime)
        return;

    if (Is_TargetInHitRange(owner, target))
        Apply_ChibakuHit(owner, target);

    _impactApplied = true;
}

bool BTTask_ChibakuTensei::Is_TargetInHitRange(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
{
    auto ownerTransform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto targetTransform = target ? target->Get_Component<Transform>() : nullptr;

    if (!ownerTransform || !targetTransform)
        return false;

    Vec3 delta = targetTransform->Get_WorldPosition() - ownerTransform->Get_WorldPosition();
    delta.y = 0.f;

    return delta.LengthSquared() <= _hitRange * _hitRange;
}

void BTTask_ChibakuTensei::Apply_ChibakuHit(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
{
    auto ownerTransform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto targetTransform = target ? target->Get_Component<Transform>() : nullptr;

    if (!ownerTransform || !targetTransform)
        return;

    auto character = static_pointer_cast<Character>(target);
    if (!character)
        return;

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 hitDir = targetPos - ownerPos;
    hitDir.y = 0.f;
    hitDir = Utils::Safe_Normalize(hitDir, ownerTransform->Get_WorldForward());

    static uint32 sChibakuHitSerial = 0;
    ++sChibakuHitSerial;

    FDamageEvent damageEvent{};
    damageEvent.damage = _damage;
    damageEvent.damageCauser = owner;
    damageEvent.hasCustomDir = true;
    damageEvent.damageDir = hitDir;
    damageEvent.launchPower = _launchPower;
    damageEvent.launchUp = _launchUp;
    damageEvent.hitReactionType = EHitReactionType::BlowOff;
    damageEvent.hitReactionSerial = sChibakuHitSerial;
    damageEvent.hitAnimStateOverride = _hitAnimStateOverride;
    damageEvent.forceHitRestart = true;

    character->TakeDamage(damageEvent);

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.collisionPreset = Collision_Preset::Monster_Attack;
    desc.startAttached = true;
    desc.spawnPosition = targetTransform->Get_WorldPosition();
    desc.ownerObject = owner;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_CHIBAKU_TENSEI,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    auto chibaku = dynamic_pointer_cast<Skill_ChibakuTensei>(spawned);
    if (chibaku)
    {
        chibaku->Set_Owner(owner);
        chibaku->Begin_AttachSequence(character.get());
    }
}

void BTTask_ChibakuTensei::Finish_Chibaku(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner, bool writeCooldown)
{
    if (blackboard)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        if (writeCooldown)
        {
            blackboard->Set_ValueAsFloat(_cooldownRemainKey, _cooldown);

            if (!_globalCooldownRemainKey.empty())
                blackboard->Set_ValueAsFloat(_globalCooldownRemainKey, _globalCooldown);
        }
    }

    auto movement = owner ? owner->Get_Component<MovementComponent>() : nullptr;
    if (movement)
    {
        movement->Set_OrientRotationToMovement(_previousOrientRotationToMovement);
        movement->Resolve_CharacterBodyPenetration();
    }

    _phase = EChibakuTenseiTaskPhase::Approach;
    _phaseElapsed = 0.f;
    _impactApplied = false;
}

void BTTask_ChibakuTensei::Request_AnimState(const Shared<Blackboard>& blackboard, const string& animState) const
{
    if (!blackboard || animState.empty())
        return;

    blackboard->Set_ValueAsString("AnimState", animState);

    const int32 nextReplaySerial = blackboard->HasKey("AnimReplaySerial")
        ? blackboard->Get_ValueAsInt("AnimReplaySerial") + 1
        : 1;

    blackboard->Set_ValueAsInt("AnimReplaySerial", nextReplaySerial);
}

void BTTask_ChibakuTensei::Face_Target(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
{
    auto ownerTransform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto targetTransform = target ? target->Get_Component<Transform>() : nullptr;

    if (!ownerTransform || !targetTransform)
        return;

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    Vec3 targetPos = targetTransform->Get_WorldPosition();
    targetPos.y = ownerPos.y;

    Vec3 lookDir = Utils::Safe_Normalize(targetPos - ownerPos, ownerTransform->Get_WorldForward());
    ownerTransform->LookAt(ownerPos + lookDir);
}

Shared<BTTask_ChibakuTensei> BTTask_ChibakuTensei::Create()
{
    return make_shared<BTTask_ChibakuTensei>();
}

Shared<BTNode> BTTask_ChibakuTensei::Clone()
{
    auto clone = make_shared<BTTask_ChibakuTensei>(*this);
    clone->Initialize();

    return clone;
}
