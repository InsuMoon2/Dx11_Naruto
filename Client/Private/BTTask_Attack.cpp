#include "pch.h"
#include "BTTask_Attack.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "AnimationStateComponent.h"

IMPLEMENT_REFLECTION(BTTask_Attack)

bool BTTask_Attack::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_Attack";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Attack Anim State", "attack_anim_state", _attackAnimState);
    PROPERTY_FLOAT_JSON("Attack Range", "attack_range", _attackRange, 0.1f, 100.f);
    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);

    return true;
}

BTTask_Attack::BTTask_Attack()
{
}

BTTask_Attack::BTTask_Attack(const BTTask_Attack& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _attackAnimState(rhs._attackAnimState)
    , _attackRange(rhs._attackRange)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _startedAttack(false)
{

}

void BTTask_Attack::Initialize()
{
    BTTask::Initialize();

    // 공격을 실행할 때마다 애니메이션 시작 여부 초기화 해야함
    _startedAttack = false;
}

EBTNodeResult BTTask_Attack::Update(float timeDelta)
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
    auto animState = owner->Get_Component<AnimationStateComponent>();

    if (!ownerTransform || !targetTransform || !animState)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 toTarget = targetPos - ownerPos;
    toTarget.y = 0.f;

    if (!_startedAttack)
    {
        float distance = toTarget.Length();
        if (distance > _attackRange)
        {
            // 공격 사정거리 밖이면, 실패를 반환해서 상위 BT가 다시 MoveTo를 타게 만들어야함
            _startedAttack = false;
            _lastResult = EBTNodeResult::Failed;

            return _lastResult;
        }

        if (_faceTarget)
        {
            // 회전
            Vec3 lookDir = Utils::Safe_Normalize(toTarget, ownerTransform->Get_WorldForward());
            ownerTransform->LookAt(ownerPos + lookDir);
        }

        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
        blackboard->Set_ValueAsString("AnimState", _attackAnimState);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);

        _startedAttack = true;
    }

    // 공격 애니메이션이 끝날 떄까지는 진행중으로
    if (!animState->Is_CurrentStateFinished())
    {
        _lastResult = EBTNodeResult::InProgress;

        return _lastResult;
    }

    // 공격 종료
    _startedAttack = false;
    _lastResult = EBTNodeResult::Succeeded;

    return _lastResult;
}

Shared<BTTask_Attack> BTTask_Attack::Create()
{
    return make_shared<BTTask_Attack>();
}

Shared<BTNode> BTTask_Attack::Clone()
{
    auto clone = make_shared<BTTask_Attack>(*this);
    clone->Initialize();
    return clone;
}
