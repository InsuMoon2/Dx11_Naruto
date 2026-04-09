#include "pch.h"
#include "BTDecorator.h"

BTDecorator::BTDecorator()
{
}

BTDecorator::BTDecorator(const BTDecorator& rhs)
    : BTNode(rhs)
{
}

void BTDecorator::Initialize()
{
    BTNode::Initialize();

    if (_child)
        _child->Initialize();
}

void BTDecorator::OnTerminate(EBTNodeResult result)
{
    _lastResult = EBTNodeResult::Failed;

    if (_child)
    {
        _child->OnTerminate(result);
    }
}

void BTDecorator::Gather_NodeResults(map<int, EBTNodeResult>& outResults)
{
    BTNode::Gather_NodeResults(outResults);

    if (_child)
        _child->Gather_NodeResults(outResults);
}

void BTDecorator::Set_Blackboard(Shared<Blackboard> blackboard)
{
    BTNode::Set_Blackboard(blackboard);

    if (_child)
        _child->Set_Blackboard(blackboard);
}

void BTDecorator::Set_Owner(Shared<GameObject> owner)
{
    BTNode::Set_Owner(owner);

    if (_child)
        _child->Set_Owner(owner);
}
