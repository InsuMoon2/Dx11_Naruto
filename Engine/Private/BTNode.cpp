#include "pch.h"
#include "BtNode.h"

BTNode::BTNode()
{

}

BTNode::BTNode(const BTNode& rhs)
    : Base(rhs)
    , _lastResult(EBTNodeResult::Failed)
    , _debugId(rhs._debugId)
{

}

BTNode::~BTNode()
{
}

EBTNodeResult BTNode::Update(float timeDelta)
{

    return EBTNodeResult::Succeeded;
}
