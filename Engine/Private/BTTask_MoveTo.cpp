#include "pch.h"
#include "BTTask_MoveTo.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"

IMPLEMENT_REFLECTION(BTTask_MoveTo)

bool BTTask_MoveTo::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_Move To";

    PROPERTY_STRING("Target Object", _targetObjectKey);
    PROPERTY_STRING("Target Location", _targetLocationKey);
    PROPERTY_FLOAT("Acceptance Radius", _acceptanceRadius, 0.1f, 100.f);
    PROPERTY_BOOL("Use Sprint", _useSprint);
    PROPERTY_BOOL("Use Move Anim", _controlMoveAnim);
    PROPERTY_STRING("Move Anim State", _moveAnimState);
    PROPERTY_STRING("Idle Anim State", _idleAnimState);
    PROPERTY_BOOL("Write Anim Direction", _writeAnimDirection);

    return true;
}

BTTask_MoveTo::BTTask_MoveTo()
{
}

BTTask_MoveTo::BTTask_MoveTo(const BTTask_MoveTo& rhs)
    : BTTask(rhs)
    , _targetLocationKey(rhs._targetLocationKey)
    , _acceptanceRadius(rhs._acceptanceRadius)
{

}

void BTTask_MoveTo::Initialize()
{
    BTTask::Initialize();

    
}

EBTNodeResult BTTask_MoveTo::Update(float timeDelta)
{
    BTTask::Update(timeDelta);

    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return EBTNodeResult::Failed;

    Vec3 targetPos = blackboard->Get_ValueAsVector(_targetLocationKey);
    Vec3 currentPos = owner->Get_Component<Transform>()->Get_LocalPosition();

    Vec3 direction = targetPos - currentPos;
    float distance = direction.Length();

    // 목표 지점 도착
    if (distance < _acceptanceRadius)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        _lastResult = EBTNodeResult::Succeeded;

        return _lastResult;
    }

    direction.Normalize();
    blackboard->Set_ValueAsFloat("MoveAxisX", direction.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", direction.z);
    blackboard->Set_ValueAsBool("Sprint", false);

    _lastResult = EBTNodeResult::InProgress;

    return _lastResult;
}

Shared<BTTask_MoveTo> BTTask_MoveTo::Create()
{
    return make_shared<BTTask_MoveTo>();
}

Shared<BTNode> BTTask_MoveTo::Clone()
{
    auto clone = make_shared<BTTask_MoveTo>(*this);

    clone->Initialize();

    return clone;
}
