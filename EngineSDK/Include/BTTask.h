#pragma once

#include "BTNode.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask : public BTNode
{
public:
    explicit BTTask();
    virtual ~BTTask() = default;

    // Task는 자식 노드를 가질 수 없다. (Leaf 노드)

};

NS_END
