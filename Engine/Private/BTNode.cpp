#include "pch.h"
#include "BtNode.h"

BTNode::BTNode()
{

}

BTNode::BTNode(const BTNode& rhs)
    : Base(rhs)
    , _lastResult(EBTNodeResult::NotExecuted)
    , _debugId(rhs._debugId)
{

}

BTNode::~BTNode()
{
}

void BTNode::Initialize()
{
    _lastResult = EBTNodeResult::NotExecuted;
}

EBTNodeResult BTNode::Update(float timeDelta)
{

    return EBTNodeResult::Succeeded;
}

void BTNode::Gather_NodeResults(map<int, EBTNodeResult>& outResults)
{
    if (_debugId != -1)
        outResults[_debugId] = _lastResult;
}
