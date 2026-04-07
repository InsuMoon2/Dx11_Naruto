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

    PROPERTY_STRING_JSON("Target Object", "target_object", _targetObjectKey);
    PROPERTY_STRING_JSON("Target Location", "target_location", _targetLocationKey);
    PROPERTY_FLOAT_JSON("Acceptance Radius", "acceptance_radius", _acceptanceRadius, 0.1f, 100.f);
    PROPERTY_BOOL_JSON("Use Sprint", "use_sprint", _useSprint);
    PROPERTY_BOOL_JSON("Use Move Anim", "use_move_anim", _controlMoveAnim);
    PROPERTY_STRING_JSON("Move Anim State", "move_anim_state", _moveAnimState);
    PROPERTY_STRING_JSON("Idle Anim State", "idle_anim_state", _idleAnimState);
    PROPERTY_BOOL_JSON("Write Anim Direction", "write_anim_direction", _writeAnimDirection);

    return true;
}

BTTask_MoveTo::BTTask_MoveTo()
{
}

BTTask_MoveTo::BTTask_MoveTo(const BTTask_MoveTo& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _targetLocationKey(rhs._targetLocationKey)
    , _acceptanceRadius(rhs._acceptanceRadius)
    , _useSprint(rhs._useSprint)
    , _controlMoveAnim(rhs._controlMoveAnim)
    , _moveAnimState(rhs._moveAnimState)
    , _idleAnimState(rhs._idleAnimState)
    , _writeAnimDirection(rhs._writeAnimDirection)
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

    auto ownerTransform = owner->Get_Component<Transform>();
    if (!ownerTransform)
        return EBTNodeResult::Failed;

    Vec3 targetPos = blackboard->Get_ValueAsVector(_targetLocationKey);

    // TargetObject가 있으면, 그 오브젝트 위치를 우선으로 따라가기
    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (targetObject)
    {
        auto targetTransform = targetObject->Get_Transform();
        if (!targetTransform)
            return EBTNodeResult::Failed;

        targetPos = targetTransform->Get_WorldPosition();

        // 현재 추적중인 실제 위치를 TargetLocation에도 같이 갱신
        blackboard->Set_ValueAsVector(_targetLocationKey, targetPos);
    }

    Vec3 currentPos = owner->Get_Component<Transform>()->Get_WorldPosition();

    Vec3 direction = targetPos - currentPos;
    direction.y = 0.f;

    float distance = direction.Length();

    // 목표 지점 도착  
    if (distance < _acceptanceRadius)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        if (_controlMoveAnim)
            blackboard->Set_ValueAsString("AnimState", _idleAnimState);

        _lastResult = EBTNodeResult::Succeeded;

        return _lastResult;
    }

    direction = Utils::Safe_Normalize(direction, ownerTransform->Get_WorldForward());

    ownerTransform->LookAt(currentPos + direction);


    blackboard->Set_ValueAsFloat("MoveAxisX", direction.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", direction.z);
    blackboard->Set_ValueAsBool("Sprint", _useSprint);

    if (_controlMoveAnim)
    {
        blackboard->Set_ValueAsString("AnimState", _moveAnimState);
    }

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
