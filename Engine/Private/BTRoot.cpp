#include "pch.h"
#include "BTRoot.h"

EBTNodeResult BTRoot::Update(float timeDelta)
{
    if (_children.empty())
        return BTComposite::Update(timeDelta);

    _lastResult = _children[0]->Update(timeDelta);

    return _lastResult;
}

Shared<BTNode> BTRoot::Clone()
{
    auto newNode = make_shared<BTRoot>();

    newNode->Set_DebugId(_debugId);

    for (auto& child : _children)
        newNode->Add_Child(child->Clone());

    return newNode;
}
