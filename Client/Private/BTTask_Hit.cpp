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
    PROPERTY_STRING_JSON("Hit Anim State", "hit_anim_state", _hitAnimState);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    return true;
}

BTTask_Hit::BTTask_Hit()
{
}

BTTask_Hit::BTTask_Hit(const BTTask_Hit& rhs)
    : BTTask(rhs)
    , _hitFlagKey(rhs._hitFlagKey)
    , _hitAnimState(rhs._hitAnimState)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _startedHit(false)
{
}

BTTask_Hit::~BTTask_Hit()
{
}

void BTTask_Hit::Initialize()
{
    BTTask::Initialize();

    _startedHit = false;
}

EBTNodeResult BTTask_Hit::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    auto animState = owner->Get_Component<AnimationStateComponent>();

    if (!blackboard || !owner || !animState)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    // 처음 Hit 애니메이션 진입
    if (!_startedHit)
    {
        // MoveAxis를 근데 .. Launch값마다 밀리게 하는건 비헤이비어트리에서 제어할 필요가있나
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);

        blackboard->Set_ValueAsString("AnimState", _hitAnimState);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);

        _startedHit = true;

        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    // 애니메이션 끝날때까지 대기
    if (!animState->Is_CurrentStateFinished())
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }
    // 애니메이션 종료
    _startedHit = false;

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
