#include "pch.h"
#include "BTTask_Hit.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "AnimationStateComponent.h"

IMPLEMENT_REFLECTION(BTTask_Hit)

bool BTTask_Hit::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_Hit";

    PROPERTY_STRING_JSON("Hit Flag Key", "hit_flag_key", _hitFlagKey);
    PROPERTY_STRING_JSON("Hit Anim State Key", "hit_anim_state_key", _hitAnimStateKey);
    PROPERTY_STRING_JSON("Hit Serial Key", "hit_serial_key", _hitSerialKey);
    PROPERTY_STRING_JSON("Anim Replay Serial Key", "anim_replay_serial_key", _animReplaySerialKey);
    PROPERTY_STRING_JSON("Default Hit Anim State", "default_hit_anim_state", _defaultHitAnimState);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);

    return true;
}

BTTask_Hit::BTTask_Hit()
{
}

BTTask_Hit::BTTask_Hit(const BTTask_Hit& rhs)
    : BTTask(rhs)
    , _hitFlagKey(rhs._hitFlagKey)
    , _hitAnimStateKey(rhs._hitAnimStateKey)
    , _hitSerialKey(rhs._hitSerialKey)
    , _animReplaySerialKey(rhs._animReplaySerialKey)
    , _defaultHitAnimState(rhs._defaultHitAnimState)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _startedHit(false)
    , _activeHitSerial(0)
{
}

BTTask_Hit::~BTTask_Hit()
{
}

void BTTask_Hit::Initialize()
{
    BTTask::Initialize();

    _startedHit = false;
    _activeHitSerial = 0;
}

string BTTask_Hit::Resolve_HitAnimState(const Shared<Blackboard>& blackboard) const
{
    if (!blackboard)
        return _defaultHitAnimState;

    if (blackboard->HasKey(_hitAnimStateKey))
    {
        const string hitAnimState = blackboard->Get_ValueAsString(_hitAnimStateKey);
        if (!hitAnimState.empty())
            return hitAnimState;
    }

    return _defaultHitAnimState;
}

EBTNodeResult BTTask_Hit::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto animState = owner->Get_Component<AnimationStateComponent>();
    if (!animState)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    const bool isHit = blackboard->Get_ValueAsBool(_hitFlagKey);
    if (!isHit)
    {
        _startedHit = false;
        _activeHitSerial = 0;
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    const int32 incomingHitSerial = blackboard->HasKey(_hitSerialKey)
        ? blackboard->Get_ValueAsInt(_hitSerialKey)
        : 0;

    const bool shouldStartHit =
        (!_startedHit) || (incomingHitSerial != _activeHitSerial);

    if (shouldStartHit)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        const string hitAnimState = Resolve_HitAnimState(blackboard);
        blackboard->Set_ValueAsString("AnimState", hitAnimState);

        // 같은 AnimState라도 serial을 올려서 AIController가 다시 재생
        blackboard->Set_ValueAsInt(_animReplaySerialKey, incomingHitSerial);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);

        _startedHit = true;
        _activeHitSerial = incomingHitSerial;

        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    if (!animState->Is_CurrentStateFinished())
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    _startedHit = false;
    _activeHitSerial = 0;

    blackboard->Set_ValueAsBool(_hitFlagKey, false);

    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

Shared<BTTask_Hit> BTTask_Hit::Create()
{
    return make_shared<BTTask_Hit>();
}

Shared<BTNode> BTTask_Hit::Clone()
{
    auto clone = make_shared<BTTask_Hit>(*this);
    clone->Initialize();

    return clone;
}
