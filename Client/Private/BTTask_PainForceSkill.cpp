// Client/Private/BTTask_PainForceSkill.cpp
#include "pch.h"
#include "BTTask_PainForceSkill.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "Character.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "SkillObject.h"
#include "Transform.h"

IMPLEMENT_REFLECTION(BTTask_PainForceSkill)

bool BTTask_PainForceSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_PainForceSkill";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Skill Anim State", "skill_anim_state", _skillAnimState);
    PROPERTY_STRING_JSON("Cooldown Remain Key", "cooldown_remain_key", _cooldownRemainKey);
    PROPERTY_STRING_JSON("Global Cooldown Remain Key", "global_cooldown_remain_key", _globalCooldownRemainKey);
    PROPERTY_ENUM_JSON("Skill Type", "skill_type", _skillType, EPainForceSkillType);
    PROPERTY_FLOAT_JSON("Min Range", "min_range", _minRange, 0.f, 100.f);
    PROPERTY_FLOAT_JSON("Max Range", "max_range", _maxRange, 0.1f, 100.f);
    PROPERTY_FLOAT_JSON("Cooldown", "cooldown", _cooldown, 0.f, 60.f);
    PROPERTY_FLOAT_JSON("Global Cooldown", "global_cooldown", _globalCooldown, 0.f, 30.f);
    PROPERTY_FLOAT_JSON("Impact Time", "impact_time", _impactTime, 0.f, 5.f);
    PROPERTY_FLOAT_JSON("Skill Duration", "skill_duration", _skillDuration, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Launch Power", "launch_power", _launchPower, 0.f, 500.f);
    PROPERTY_FLOAT_JSON("Launch Up", "launch_up", _launchUp, -50.f, 50.f);
    PROPERTY_FLOAT_JSON("Damage", "damage", _damage, 0.f, 999.f);
    PROPERTY_STRING_JSON("Hit Anim State Override", "hit_anim_state_override", _hitAnimStateOverride);
    PROPERTY_BOOL_JSON("Affect All Players In Range", "affect_all_players_in_range", _affectAllPlayersInRange);
    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    PROPERTY_STRING_JSON("Effect Asset Name", "effect_asset_name", _effectAssetName);
    PROPERTY_VEC3_JSON("Effect Scale", "effect_scale", _effectScale, 0.1f);

    return true;
}

BTTask_PainForceSkill::BTTask_PainForceSkill()
{
}

BTTask_PainForceSkill::BTTask_PainForceSkill(const BTTask_PainForceSkill& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _skillAnimState(rhs._skillAnimState)
    , _cooldownRemainKey(rhs._cooldownRemainKey)
    , _globalCooldownRemainKey(rhs._globalCooldownRemainKey)
    , _skillType(rhs._skillType)
    , _minRange(rhs._minRange)
    , _maxRange(rhs._maxRange)
    , _cooldown(rhs._cooldown)
    , _globalCooldown(rhs._globalCooldown)
    , _impactTime(rhs._impactTime)
    , _skillDuration(rhs._skillDuration)
    , _launchPower(rhs._launchPower)
    , _launchUp(rhs._launchUp)
    , _damage(rhs._damage)
    , _hitAnimStateOverride(rhs._hitAnimStateOverride)
    , _affectAllPlayersInRange(rhs._affectAllPlayersInRange)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _effectAssetName(rhs._effectAssetName)
{
}

void BTTask_PainForceSkill::Initialize()
{
    BTTask::Initialize();

    _startedSkill = false;
    _elapsed = 0.f;
    _impactApplied = false;
}

EBTNodeResult BTTask_PainForceSkill::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    if (!_startedSkill && Tick_Cooldown(blackboard, timeDelta))
        return _lastResult = EBTNodeResult::Failed;

    if (!_startedSkill && !_globalCooldownRemainKey.empty())
    {
        const float globalCooldownRemain = blackboard->HasKey(_globalCooldownRemainKey)
            ? blackboard->Get_ValueAsFloat(_globalCooldownRemainKey)
            : 0.f;

        if (globalCooldownRemain > 0.f)
            return _lastResult = EBTNodeResult::Failed;
    }

    auto targets = Collect_Targets(owner);
    if (targets.empty())
        return _lastResult = EBTNodeResult::Failed;

    auto ownerTransform = owner->Get_Component<Transform>();
    auto animState = owner->Get_Component<AnimationStateComponent>();

    if (!ownerTransform || !animState)
        return _lastResult = EBTNodeResult::Failed;

    if (!_startedSkill)
    {
        if (_faceTarget)
        {
            auto targetTransform = targets.front()->Get_Component<Transform>();
            if (targetTransform)
            {
                Vec3 ownerPos = ownerTransform->Get_WorldPosition();
                Vec3 targetPos = targetTransform->Get_WorldPosition();
                targetPos.y = ownerPos.y;

                Vec3 lookDir = Utils::Safe_Normalize(targetPos - ownerPos, ownerTransform->Get_WorldForward());
                ownerTransform->LookAt(ownerPos + lookDir);
            }
        }

        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        Request_SkillAnimation(blackboard);

        if (!_effectAssetName.empty())
        {
            Vec3 effectPos = ownerTransform->Get_WorldPosition();
            SkillObject::Spawn_Effect_Once(_effectAssetName, effectPos, _effectScale);
        }

        _startedSkill = true;
        _elapsed = 0.f;
        _impactApplied = false;

        return _lastResult = EBTNodeResult::InProgress;
    }

    _elapsed += timeDelta;

    if (!_impactApplied && _elapsed >= _impactTime)
    {
        if (_affectAllPlayersInRange)
        {
            for (const auto& target : targets)
                Apply_ForceToTarget(owner, target);
        }
        else
        {
            Apply_ForceToTarget(owner, targets.front());
        }

        _impactApplied = true;
    }

    if (_elapsed < _skillDuration)
        return _lastResult = EBTNodeResult::InProgress;

    if (!animState->Is_CurrentStateFinished())
        return _lastResult = EBTNodeResult::InProgress;

    blackboard->Set_ValueAsFloat(_cooldownRemainKey, _cooldown);

    if (!_globalCooldownRemainKey.empty())
        blackboard->Set_ValueAsFloat(_globalCooldownRemainKey, _globalCooldown);

    _startedSkill = false;
    _elapsed = 0.f;
    _impactApplied = false;

    return _lastResult = EBTNodeResult::Succeeded;
}

bool BTTask_PainForceSkill::Tick_Cooldown(const Shared<Blackboard>& blackboard, float timeDelta)
{
    if (_cooldownRemainKey.empty())
        return false;

    float remain = blackboard->HasKey(_cooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_cooldownRemainKey)
        : 0.f;

    if (remain <= 0.f)
        return false;

    remain = max(0.f, remain - timeDelta);
    blackboard->Set_ValueAsFloat(_cooldownRemainKey, remain);

    return remain > 0.f;
}

vector<Shared<GameObject>> BTTask_PainForceSkill::Collect_Targets(const Shared<GameObject>& owner) const
{
    vector<Shared<GameObject>> targets;

    auto ownerTransform = owner->Get_Component<Transform>();
    if (!ownerTransform)
        return targets;

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const float minRangeSq = _minRange * _minRange;
    const float maxRangeSq = _maxRange * _maxRange;

    auto objects = GAME->Get_GameObjects(GAME->Current_Level());
    for (const auto& obj : objects)
    {
        if (!obj || obj == owner || obj->Is_Destroy())
            continue;

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER &&
            obj->Get_ObjectType() != Protocol::OBJECT_TYPE_REMOTE_PLAYER)
            continue;

        auto targetTransform = obj->Get_Component<Transform>();
        if (!targetTransform)
            continue;

        Vec3 toTarget = targetTransform->Get_WorldPosition() - ownerPos;
        const float distSq = toTarget.LengthSquared();

        if (distSq < minRangeSq || distSq > maxRangeSq)
            continue;

        targets.push_back(obj);
    }

    return targets;
}

void BTTask_PainForceSkill::Request_SkillAnimation(const Shared<Blackboard>& blackboard)
{
    blackboard->Set_ValueAsString("AnimState", _skillAnimState);

    const int32 nextReplaySerial = blackboard->HasKey("AnimReplaySerial")
        ? blackboard->Get_ValueAsInt("AnimReplaySerial") + 1
        : 1;

    blackboard->Set_ValueAsInt("AnimReplaySerial", nextReplaySerial);

    if (_requestAnimEnd)
        blackboard->Set_ValueAsBool("AnimRequestEnd", true);
}

void BTTask_PainForceSkill::Apply_ForceToTarget(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
{
    auto ownerTransform = owner->Get_Component<Transform>();
    auto targetTransform = target->Get_Component<Transform>();

    if (!ownerTransform || !targetTransform)
        return;

    auto character = static_pointer_cast<Character>(target);
    if (!character)
        return;

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 forceDir = Vec3::Zero;

    if (_skillType == EPainForceSkillType::BanshoTenin)
        forceDir = ownerPos - targetPos;
    else
        forceDir = targetPos - ownerPos;

    forceDir.y = 0.f;
    forceDir = Utils::Safe_Normalize(forceDir, ownerTransform->Get_WorldForward());

    static uint32 sPainForceHitSerial = 0;
    ++sPainForceHitSerial;

    FDamageEvent damageEvent{};
    damageEvent.damage = _damage;
    damageEvent.damageCauser = owner;
    damageEvent.hasCustomDir = true;
    damageEvent.damageDir = forceDir;
    damageEvent.launchPower = _launchPower;
    damageEvent.launchUp = _launchUp;
    damageEvent.hitReactionType = (_skillType == EPainForceSkillType::BanshoTenin)
        ? EHitReactionType::Stagger
        : EHitReactionType::BlowOff;
    damageEvent.hitReactionSerial = sPainForceHitSerial;
    damageEvent.hitAnimStateOverride = _hitAnimStateOverride;
    damageEvent.forceHitRestart = true;

    character->TakeDamage(damageEvent);
}

Shared<BTTask_PainForceSkill> BTTask_PainForceSkill::Create()
{
    return make_shared<BTTask_PainForceSkill>();
}

Shared<BTNode> BTTask_PainForceSkill::Clone()
{
    auto clone = make_shared<BTTask_PainForceSkill>(*this);
    clone->Initialize();

    return clone;
}
