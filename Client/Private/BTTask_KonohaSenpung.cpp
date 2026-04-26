#include "pch.h"
#include "BTTask_KonohaSenpung.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_KonohaSenpung)

bool BTTask_KonohaSenpung::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_KonohaSenpung";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Skill Anim State", "skill_anim_state", _skillAnimState);
    PROPERTY_STRING_JSON("Cooldown Remain Key", "cooldown_remain_key", _cooldownRemainKey);

    PROPERTY_FLOAT_JSON("Min Range", "min_range", _minRange, 0.f, 100.f);
    PROPERTY_FLOAT_JSON("Max Range", "max_range", _maxRange, 0.1f, 100.f);
    PROPERTY_FLOAT_JSON("Cooldown", "cooldown", _cooldown, 0.f, 60.f);
    PROPERTY_FLOAT_JSON("Skill Duration", "skill_duration", _skillDuration, 0.1f, 10.f);

    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);

    return true;
}

BTTask_KonohaSenpung::BTTask_KonohaSenpung()
{
}

BTTask_KonohaSenpung::BTTask_KonohaSenpung(const BTTask_KonohaSenpung& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _skillAnimState(rhs._skillAnimState)
    , _cooldownRemainKey(rhs._cooldownRemainKey)
    , _minRange(rhs._minRange)
    , _maxRange(rhs._maxRange)
    , _cooldown(rhs._cooldown)
    , _skillDuration(rhs._skillDuration)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
{
}

void BTTask_KonohaSenpung::Initialize()
{
    BTTask::Initialize();

    _startedSkill = false;
    _elapsed = 0.f;
}

EBTNodeResult BTTask_KonohaSenpung::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    auto target = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!target || target->Is_Destroy())
    {
        Finish_Skill(blackboard, false);
        return _lastResult = EBTNodeResult::Failed;
    }

    if (!_startedSkill)
    {
        if (Tick_Cooldown(blackboard, timeDelta))
            return _lastResult = EBTNodeResult::Failed;

        if (!Is_TargetInRange(owner, target))
            return _lastResult = EBTNodeResult::Failed;

        Begin_Skill(blackboard, owner, target);
        return _lastResult = EBTNodeResult::InProgress;
    }

    _elapsed += timeDelta;

    auto animState = owner->Get_Component<AnimationStateComponent>();
    const bool timeFinished = _elapsed >= _skillDuration;
    const bool animFinished = !animState || animState->Is_CurrentStateFinished();

    if (!timeFinished || !animFinished)
        return _lastResult = EBTNodeResult::InProgress;

    Finish_Skill(blackboard, true);
    return _lastResult = EBTNodeResult::Succeeded;
}

bool BTTask_KonohaSenpung::Tick_Cooldown(const Shared<Blackboard>& blackboard, float timeDelta)
{
    if (!blackboard || _cooldownRemainKey.empty())
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

bool BTTask_KonohaSenpung::Is_TargetInRange(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
{
    auto ownerTransform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto targetTransform = target ? target->Get_Component<Transform>() : nullptr;

    if (!ownerTransform || !targetTransform)
        return false;

    Vec3 delta = targetTransform->Get_WorldPosition() - ownerTransform->Get_WorldPosition();
    delta.y = 0.f;

    const float distSq = delta.LengthSquared();
    const float minRangeSq = _minRange * _minRange;
    const float maxRangeSq = _maxRange * _maxRange;

    return distSq >= minRangeSq && distSq <= maxRangeSq;
}

void BTTask_KonohaSenpung::Begin_Skill(
    const Shared<Blackboard>& blackboard,
    const Shared<GameObject>& owner,
    const Shared<GameObject>& target)
{
    Stop_Move(blackboard);

    if (_faceTarget)
        Face_Target(owner, target);

    Request_SkillAnimation(blackboard);

    _startedSkill = true;
    _elapsed = 0.f;
}

void BTTask_KonohaSenpung::Finish_Skill(const Shared<Blackboard>& blackboard, bool succeeded)
{
    Stop_Move(blackboard);

    if (blackboard && succeeded && !_cooldownRemainKey.empty())
        blackboard->Set_ValueAsFloat(_cooldownRemainKey, _cooldown);

    _startedSkill = false;
    _elapsed = 0.f;
}

void BTTask_KonohaSenpung::Request_SkillAnimation(const Shared<Blackboard>& blackboard) const
{
    if (!blackboard || _skillAnimState.empty())
        return;

    blackboard->Set_ValueAsString("AnimState", _skillAnimState);

    const int32 nextReplaySerial = blackboard->HasKey("AnimReplaySerial")
        ? blackboard->Get_ValueAsInt("AnimReplaySerial") + 1
        : 1;

    blackboard->Set_ValueAsInt("AnimReplaySerial", nextReplaySerial);

    if (_requestAnimEnd)
        blackboard->Set_ValueAsBool("AnimRequestEnd", true);
}

void BTTask_KonohaSenpung::Face_Target(const Shared<GameObject>& owner, const Shared<GameObject>& target) const
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

void BTTask_KonohaSenpung::Stop_Move(const Shared<Blackboard>& blackboard) const
{
    if (!blackboard)
        return;

    blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
    blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
    blackboard->Set_ValueAsBool("Sprint", false);
}

Shared<BTTask_KonohaSenpung> BTTask_KonohaSenpung::Create()
{
    return make_shared<BTTask_KonohaSenpung>();
}

Shared<BTNode> BTTask_KonohaSenpung::Clone()
{
    auto clone = make_shared<BTTask_KonohaSenpung>(*this);
    clone->Initialize();

    return clone;
}
