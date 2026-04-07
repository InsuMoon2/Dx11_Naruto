#include "pch.h"
#include "BTTask_FindClosestTarget.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "GameInstance.h"

bool BTTask_FindClosestTarget::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_FindClosestTarget";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Target Location Key", "target_location_key", _targetLocationKey);
    PROPERTY_FLOAT_JSON("Search Radius", "search_radius", _searchRadius, 0.1f, 9999.f);

    return true;
}

BTTask_FindClosestTarget::BTTask_FindClosestTarget()
{
}

BTTask_FindClosestTarget::BTTask_FindClosestTarget(const BTTask_FindClosestTarget& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _targetLocationKey(rhs._targetLocationKey)
    , _searchRadius(rhs._searchRadius)
{
}

void BTTask_FindClosestTarget::Initialize()
{
    BTTask::Initialize();
}

EBTNodeResult BTTask_FindClosestTarget::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto ownerTransform = owner->Get_Component<Transform>();
    if (!ownerTransform)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const float searchRadiusSq = _searchRadius * _searchRadius;

    Shared<GameObject> closestTarget = nullptr;
    float closestDistSq = searchRadiusSq;

    auto objects = GAME->Get_GameObjects(GAME->Current_Level());

    for (const auto& obj : objects)
    {
        if (!obj || obj == owner || obj->Is_Destroy())
            continue;

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER)
            continue;
            //|| obj->Get_ObjectType() != Protocol::OBJECT_TYPE_REMOTE_PLAYER)

        auto targetTransform = obj->Get_Transform();
        if (!targetTransform)
            continue;

        Vec3 dir = targetTransform->Get_WorldPosition() - ownerPos;
        float distSq = dir.LengthSquared();

        if (distSq > closestDistSq)
            continue;

        closestDistSq = distSq;
        closestTarget = obj;
    }

    if (!closestTarget)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    blackboard->Set_ValueAsObject(_targetObjectKey, closestTarget);
    blackboard->Set_ValueAsVector(
        _targetLocationKey,
        closestTarget->Get_Transform()->Get_WorldPosition());

    _lastResult = EBTNodeResult::Succeeded;

    return _lastResult;
}

Shared<BTTask_FindClosestTarget> BTTask_FindClosestTarget::Create()
{
    return make_shared<BTTask_FindClosestTarget>();
}

Shared<BTNode> BTTask_FindClosestTarget::Clone()
{
    auto clone = make_shared<BTTask_FindClosestTarget>(*this);

    clone->Initialize();

    return clone;
}
