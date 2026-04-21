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

    PROPERTY_STRING_JSON("Attack Anim State 01", "attack_anim_state_01", _attackAnimState01);
    PROPERTY_STRING_JSON("Attack Anim State 02", "attack_anim_state_02", _attackAnimState02);
    PROPERTY_STRING_JSON("Attack Anim State 03", "attack_anim_state_03", _attackAnimState03);
    PROPERTY_STRING_JSON("Attack Anim State 04", "attack_anim_state_03", _attackAnimState04);

    PROPERTY_FLOAT_JSON("Attack Range", "attack_range", _attackRange, 0.1f, 100.f);
    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);

    PROPERTY_STRING_JSON("Attack Cycle Index Key", "attack_cycle_index_key", _attackCycleIndexKey);
    PROPERTY_BOOL_JSON("Use Round Robin", "use_round_robin", _useRoundRobin);

    return true;
}

BTTask_Attack::BTTask_Attack()
{
}

BTTask_Attack::BTTask_Attack(const BTTask_Attack& rhs)
    : BTTask(rhs)
   , _targetObjectKey(rhs._targetObjectKey)
    , _attackAnimState01(rhs._attackAnimState01)
    , _attackAnimState02(rhs._attackAnimState02)
    , _attackAnimState03(rhs._attackAnimState03)
    , _attackAnimState04(rhs._attackAnimState04)
    , _attackRange(rhs._attackRange)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _attackCycleIndexKey(rhs._attackCycleIndexKey)
    , _useRoundRobin(rhs._useRoundRobin)
    , _selectedAttackAnimState(rhs._selectedAttackAnimState)
    , _startedAttack(false)
{

}

void BTTask_Attack::Initialize()
{
    BTTask::Initialize();

    // 공격을 실행할 때마다 애니메이션 시작 여부 초기화 해야함
    _startedAttack = false;

    _selectedAttackAnimState.clear();
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

        _selectedAttackAnimState = Select_AttackAnimState(blackboard);

        if (_selectedAttackAnimState.empty())
        {
            _lastResult = EBTNodeResult::Failed;

            return _lastResult;
        }

        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        blackboard->Set_ValueAsString("AnimState", _selectedAttackAnimState);

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

    // 공격 종료 시 상태 비우기
    _startedAttack = false;
    _selectedAttackAnimState.clear();

    _lastResult = EBTNodeResult::Succeeded;

    return _lastResult;
}

vector<string> BTTask_Attack::Build_AttackAnimStateList() const
{
    vector<string> attackStates;

    if (!_attackAnimState01.empty())
        attackStates.push_back(_attackAnimState01);

    if (!_attackAnimState02.empty())
        attackStates.push_back(_attackAnimState02);

    if (!_attackAnimState03.empty())
        attackStates.push_back(_attackAnimState03);

      if (!_attackAnimState04.empty())
        attackStates.push_back(_attackAnimState04);

    return attackStates;
}

string BTTask_Attack::Select_AttackAnimState(const Shared<Blackboard>& blackboard)
{
    const vector<string> attackStates = Build_AttackAnimStateList();

    // 단일 공격만 할 때 -> 거의 사용안함 Legacy 호환
    if (!_useRoundRobin)
    {
        if (!_attackAnimState01.empty())
            return _attackAnimState01;

        return attackStates.empty() ? "" : attackStates.front();
    }

    if (attackStates.empty())
        return _attackAnimState01.empty() ? "" : _attackAnimState01;

    int32 nextAttackIndex = 0;

    if (blackboard && blackboard->HasKey(_attackCycleIndexKey))
    {
        nextAttackIndex = blackboard->Get_ValueAsInt(_attackCycleIndexKey);
    }

    if (nextAttackIndex < 0)
        nextAttackIndex = 0;

    const int32 selectedIndex = nextAttackIndex % static_cast<int32>(attackStates.size());
    const int32 nextIndex = (selectedIndex + 1) % static_cast<int32>(attackStates.size());

    // 다음 공격 진입 미리 기록해놓기
     if (blackboard)
        blackboard->Set_ValueAsInt(_attackCycleIndexKey, nextIndex);

    return attackStates[selectedIndex];
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
