#include "pch.h"
#include "BTComposite.h"

// ================= BTComposite =================
BTComposite::BTComposite()
{
}

void BTComposite::Add_Child(Shared<BTNode> node)
{
    _children.emplace_back(node);
}

void BTComposite::OnTerminate(EBTNodeResult result)
{
    _runningChildIndex = 0; // 상태 초기화

    // 자식들에게도 상태 전파
    for (auto& child : _children)
    {
        child->OnTerminate(result);
    }
}

void BTComposite::Set_Blackboard(Shared<Blackboard> blackboard)
{
    BTNode::Set_Blackboard(blackboard);

    for (auto& child : _children)
    {
        if (child)
            child->Set_Blackboard(blackboard);
    }
}

BTSelector::BTSelector(const BTSelector& rhs)
    : BTComposite(rhs)
{

}

// ================= BTSelector =================
EBTNodeResult BTSelector::Update(float timeDelta)
{
    if (_children.empty())
    {
        _lastResult = EBTNodeResult::Failed;

        return _lastResult;
    }

    for (int i = _runningChildIndex; i < _children.size(); i++)
    {
        EBTNodeResult result = _children[i]->Update(timeDelta);

        // 성공 -> 즉시 성공 반환 후 다른 노드 실행 XX
        if (result == EBTNodeResult::Succeeded)
        {
            _runningChildIndex = 0;

            return EBTNodeResult::Succeeded;
        }
        // 진행 중 -> 이번 프레임 리턴, 다음 프레임에 여기서부터 다시 시작
        if (result == EBTNodeResult::InProgress)
        {
            _runningChildIndex = i;

            return EBTNodeResult::InProgress;
        }

        // 실패 -> 다음 자식 (i+1)로 넘어감
    }

    // 모든 자식이 실패할 경우 -> 실패
    _runningChildIndex = 0;

    _lastResult = EBTNodeResult::Failed;
    return EBTNodeResult::Failed;
}

Shared<BTNode> BTSelector::Clone()
{
    auto newNode = make_shared<BTSelector>();

    // 자식들 깊은 복사
    for (auto& child : _children)
    {
        newNode->Add_Child(child->Clone());
    }

    return newNode;
}

BTSequence::BTSequence(const BTSequence& rhs)
{

}

// ================= BTSequence =================
EBTNodeResult BTSequence::Update(float timeDelta)
{
    if (_children.empty())
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    for (int i = _runningChildIndex; i < _children.size(); i++)
    {
        EBTNodeResult result = _children[i]->Update(timeDelta);

        // 실패 -> 즉시 실패
        if (result == EBTNodeResult::Failed)
        {
            _runningChildIndex = 0;

            return EBTNodeResult::Failed;
        }
        // 진행 중 -> 여기서 정지
        if (result == EBTNodeResult::InProgress)
        {
            _runningChildIndex = i;

            return EBTNodeResult::InProgress;
        }

        // 성공 -> 다음 자식 (i+1)로 넘어감
    }

    // 모든 자식이 성공 -> 성공
    _runningChildIndex = 0;

    _lastResult = EBTNodeResult::Succeeded;
    return EBTNodeResult::Succeeded;
}

Shared<BTNode> BTSequence::Clone()
{
    auto newNode = make_shared<BTSequence>();

    // 자식들 깊은 복사
    for (auto& child : _children)
    {
        newNode->Add_Child(child->Clone());
    }

    return newNode;
}
