#include "pch.h"
#include "BTRoot.h"

EBTNodeResult BTRoot::Update(float timeDelta)
{
    if (_children.empty())
        return BTComposite::Update(timeDelta);

    return _children[0]->Update(timeDelta);
}

Shared<BTNode> BTRoot::Clone()
{
    return make_shared<BTRoot>(*this);
}
