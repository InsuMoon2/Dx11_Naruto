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

    PROPERTY_STRING_JSON("Ground Hit Anim 01", "ground_hit_anim_state_01", _groundHitAnimState01);
    PROPERTY_STRING_JSON("Ground Hit Anim 02", "ground_hit_anim_state_02", _groundHitAnimState02);
    PROPERTY_STRING_JSON("Ground Hit Anim 03", "ground_hit_anim_state_03", _groundHitAnimState03);

    PROPERTY_STRING_JSON("Air Hit Anim 01", "air_hit_anim_state_01", _airHitAnimState01);
    PROPERTY_STRING_JSON("Air Hit Anim 02", "air_hit_anim_state_02", _airHitAnimState02);
    PROPERTY_STRING_JSON("Air Hit Anim 03", "air_hit_anim_state_03", _airHitAnimState03);

    PROPERTY_STRING_JSON("Ground Hit Cycle Index Key", "ground_hit_cycle_index_key", _groundHitCycleIndexKey);
    PROPERTY_STRING_JSON("Air Hit Cycle Index Key", "air_hit_cycle_index_key", _airHitCycleIndexKey);

    PROPERTY_ENUM_JSON("Hit Anim Select Mode", "hit_anim_select_mode", _hitAnimSelectMode, EHitAnimSelectMode);
    PROPERTY_BOOL_JSON("Use Blackboard Hit Anim Override", "use_blackboard_hit_anim_override", _useBlackboardHitAnimOverride);
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
    , _groundHitAnimState01(rhs._groundHitAnimState01)
    , _groundHitAnimState02(rhs._groundHitAnimState02)
    , _groundHitAnimState03(rhs._groundHitAnimState03)
    , _airHitAnimState01(rhs._airHitAnimState01)
    , _airHitAnimState02(rhs._airHitAnimState02)
    , _airHitAnimState03(rhs._airHitAnimState03)
    , _groundHitCycleIndexKey(rhs._groundHitCycleIndexKey)
    , _airHitCycleIndexKey(rhs._airHitCycleIndexKey)
    , _hitAnimSelectMode(rhs._hitAnimSelectMode)
    , _useBlackboardHitAnimOverride(rhs._useBlackboardHitAnimOverride)
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

        const string hitAnimState = Resolve_HitAnimState(blackboard, owner);
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

bool BTTask_Hit::Is_AirborneHit(const Shared<GameObject>& owner) const
{
    if (!owner)
        return false;

    auto movement = owner->Get_Component<MovementComponent>();
    if (!movement)
        return false;

    return !movement->Is_OnGround();

}

vector<string> BTTask_Hit::Build_HitAnimStateList(bool isAirborne) const
{
    vector<string> hitAnimStates;

    if (isAirborne)
    {
        if (!_airHitAnimState01.empty())
            hitAnimStates.push_back(_airHitAnimState01);
        if (!_airHitAnimState02.empty())
            hitAnimStates.push_back(_airHitAnimState02);
        if (!_airHitAnimState03.empty())
            hitAnimStates.push_back(_airHitAnimState03);
    }
    else
    {
        if (!_groundHitAnimState01.empty())
            hitAnimStates.push_back(_groundHitAnimState01);
        if (!_groundHitAnimState02.empty())
            hitAnimStates.push_back(_groundHitAnimState02);
        if (!_groundHitAnimState03.empty())
            hitAnimStates.push_back(_groundHitAnimState03);
    }

    return hitAnimStates;
}

string BTTask_Hit::Select_HitAnimState(const Shared<Blackboard>& blackboard, bool isAirborne) const
{
    const vector<string> hitAnimStates = Build_HitAnimStateList(isAirborne);

    if (hitAnimStates.empty())
    {
        return isAirborne ? "Hit_Air" : "Hit";
    }

    // 단일이면 맨 앞에거. 그런데 안쓸듯?
    if (_hitAnimSelectMode == EHitAnimSelectMode::Single)
        return hitAnimStates.front();

    // 세팅하기 귀찮을땐 대충넣고 랜덤
    if (_hitAnimSelectMode == EHitAnimSelectMode::Random)
    {
        const int32 randomIndex = static_cast<int32>(
            Utils::RandomRange(0.f, static_cast<float>(hitAnimStates.size())));

        const int32 safeIndex = min(
            static_cast<int32>(hitAnimStates.size()) - 1, max(0, randomIndex));

        return hitAnimStates[safeIndex];
    }

    const string& cycleIndexKey = isAirborne
        ? _airHitCycleIndexKey
        : _groundHitCycleIndexKey;

    int32 nextIndex = 0;
    if (blackboard && blackboard->HasKey(cycleIndexKey))
        nextIndex = blackboard->Get_ValueAsInt(cycleIndexKey);

    if (nextIndex < 0)
        nextIndex = 0;

    const int32 selectedIndex = nextIndex % static_cast<int32>(hitAnimStates.size());
    const int32 writeBackIndex = (selectedIndex + 1) % static_cast<int32>(hitAnimStates.size());

    if (blackboard)
        blackboard->Set_ValueAsInt(cycleIndexKey, writeBackIndex);

    return hitAnimStates[selectedIndex];
}

string BTTask_Hit::Resolve_HitAnimState(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner) const
{
    if (_useBlackboardHitAnimOverride && blackboard && blackboard->HasKey(_hitAnimStateKey))
    {
        const string hitAnimState = blackboard->Get_ValueAsString(_hitAnimStateKey);
        if (!hitAnimState.empty())
            return hitAnimState;
    }

    const bool isAirborne = Is_AirborneHit(owner);

    return Select_HitAnimState(blackboard, isAirborne);
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
