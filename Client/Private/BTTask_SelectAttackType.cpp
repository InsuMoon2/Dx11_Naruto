#include "pch.h"
#include "BTTask_SelectAttackType.h"

#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "MovementComponent.h"

IMPLEMENT_REFLECTION(BTTask_SelectAttackType)

bool BTTask_SelectAttackType::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_SelectAttackType";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Target Is Airborne Key", "target_is_airborne_key", _targetIsAirborneKey);
    PROPERTY_STRING_JSON("Target Height Delta Key", "target_height_delta_key", _targetHeightDeltaKey);
    PROPERTY_STRING_JSON("Desired Combat Mode Key", "desired_combat_mode_key", _desiredCombatModeKey);
    PROPERTY_STRING_JSON("Selected Attack State Key", "selected_attack_state_key", _selectedAttackStateKey);

    PROPERTY_STRING_JSON("Ground Attack Cycle Index Key", "ground_attack_cycle_index_key", _groundAttackCycleIndexKey);
    PROPERTY_STRING_JSON("Air Attack Cycle Index Key", "air_attack_cycle_index_key", _airAttackCycleIndexKey);

    PROPERTY_FLOAT_JSON("Airborne Height Threshold", "airborne_height_threshold", _airborneHeightThreshold, 0.1f, 10.f);

    PROPERTY_STRING_JSON("Ground Attack State 01", "ground_attack_state_01", _groundAttackState01);
    PROPERTY_STRING_JSON("Ground Attack State 02", "ground_attack_state_02", _groundAttackState02);
    PROPERTY_STRING_JSON("Ground Attack State 03", "ground_attack_state_03", _groundAttackState03);
    PROPERTY_STRING_JSON("Ground Attack State 04", "ground_attack_state_04", _groundAttackState04);

    PROPERTY_STRING_JSON("Air Attack State 01", "air_attack_state_01", _airAttackState01);
    PROPERTY_STRING_JSON("Air Attack State 02", "air_attack_state_02", _airAttackState02);
    PROPERTY_STRING_JSON("Air Attack State 03", "air_attack_state_03", _airAttackState03);
    PROPERTY_STRING_JSON("Air Attack State 04", "air_attack_state_04", _airAttackState04);
    PROPERTY_STRING_JSON("Can Use Aerial Attack Key", "can_use_aerial_attack_key", _canUseAerialAttackKey);

    return true;
}

BTTask_SelectAttackType::BTTask_SelectAttackType()
{
}

BTTask_SelectAttackType::BTTask_SelectAttackType(const BTTask_SelectAttackType& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _targetIsAirborneKey(rhs._targetIsAirborneKey)
    , _targetHeightDeltaKey(rhs._targetHeightDeltaKey)
    , _desiredCombatModeKey(rhs._desiredCombatModeKey)
    , _selectedAttackStateKey(rhs._selectedAttackStateKey)
    , _groundAttackCycleIndexKey(rhs._groundAttackCycleIndexKey)
    , _airAttackCycleIndexKey(rhs._airAttackCycleIndexKey)
    , _airborneHeightThreshold(rhs._airborneHeightThreshold)
    , _groundAttackState01(rhs._groundAttackState01)
    , _groundAttackState02(rhs._groundAttackState02)
    , _groundAttackState03(rhs._groundAttackState03)
    , _groundAttackState04(rhs._groundAttackState04)
    , _airAttackState01(rhs._airAttackState01)
    , _airAttackState02(rhs._airAttackState02)
    , _airAttackState03(rhs._airAttackState03)
    , _airAttackState04(rhs._airAttackState04)
    , _canUseAerialAttackKey(rhs._canUseAerialAttackKey)
{
}

void BTTask_SelectAttackType::Initialize()
{
    BTTask::Initialize();
}

string BTTask_SelectAttackType::Select_NextAttackState(
    const Shared<Blackboard>& blackboard,
    bool useAerialAttack) const
{
    vector<string> attackStates;
    string cycleIndexKey = _groundAttackCycleIndexKey;

    if (useAerialAttack)
    {
        cycleIndexKey = _airAttackCycleIndexKey;

        if (!_airAttackState01.empty())
            attackStates.push_back(_airAttackState01);
        if (!_airAttackState02.empty())
            attackStates.push_back(_airAttackState02);
        if (!_airAttackState03.empty())
            attackStates.push_back(_airAttackState03);
        if (!_airAttackState04.empty())
            attackStates.push_back(_airAttackState04);
    }
    else
    {
        if (!_groundAttackState01.empty())
            attackStates.push_back(_groundAttackState01);
        if (!_groundAttackState02.empty())
            attackStates.push_back(_groundAttackState02);
        if (!_groundAttackState03.empty())
            attackStates.push_back(_groundAttackState03);
        if (!_groundAttackState04.empty())
            attackStates.push_back(_groundAttackState04);
    }

    if (attackStates.empty())
        return "";

    int32 nextIndex = 0;
    if (blackboard && blackboard->HasKey(cycleIndexKey))
        nextIndex = blackboard->Get_ValueAsInt(cycleIndexKey);

    if (nextIndex < 0)
        nextIndex = 0;

    const int32 selectedIndex = nextIndex % static_cast<int32>(attackStates.size());
    const int32 writeBackIndex = (selectedIndex + 1) % static_cast<int32>(attackStates.size());

    if (blackboard)
        blackboard->Set_ValueAsInt(cycleIndexKey, writeBackIndex);

    return attackStates[selectedIndex];
}

EBTNodeResult BTTask_SelectAttackType::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!targetObject || targetObject->Is_Destroy())
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto ownerTransform = owner->Get_Component<Transform>();
    auto targetTransform = targetObject->Get_Component<Transform>();
    auto targetMovement = targetObject->Get_Component<MovementComponent>();

    if (!ownerTransform || !targetTransform)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();
    const float heightDelta = targetPos.y - ownerPos.y;

    bool targetIsAirborne = false;

    if (targetMovement)
        targetIsAirborne = !targetMovement->Is_OnGround();

    if (!targetIsAirborne && fabsf(heightDelta) >= _airborneHeightThreshold)
        targetIsAirborne = true;

    blackboard->Set_ValueAsBool(_targetIsAirborneKey, targetIsAirborne);
    blackboard->Set_ValueAsFloat(_targetHeightDeltaKey, heightDelta);

    const bool canUseAerialAttack = blackboard->HasKey(_canUseAerialAttackKey)
        ? blackboard->Get_ValueAsBool(_canUseAerialAttackKey)
        : targetIsAirborne;

    const bool useAerialAttack = targetIsAirborne && canUseAerialAttack;

    // 0 = Ground, 1 = Aerial
    blackboard->Set_ValueAsInt(_desiredCombatModeKey, useAerialAttack ? 1 : 0);

    const string selectedAttackState = Select_NextAttackState(blackboard, useAerialAttack);
    blackboard->Set_ValueAsString(_selectedAttackStateKey, selectedAttackState);

    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

Shared<BTTask_SelectAttackType> BTTask_SelectAttackType::Create()
{
    return make_shared<BTTask_SelectAttackType>();
}

Shared<BTNode> BTTask_SelectAttackType::Clone()
{
    auto clone = make_shared<BTTask_SelectAttackType>(*this);
    clone->Initialize();

    return clone;
}
