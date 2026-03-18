#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Engine)
class BTTask_Wait;
NS_END

NS_BEGIN(Editor)

class BehaviorTree_Inspector : public Component_Inspector
{
public:
    explicit BehaviorTree_Inspector() = default;
    virtual ~BehaviorTree_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_BEHAVIOR; }

private:
    void    Draw_WaitMode(Shared<BTTask_Wait> node);

};

NS_END
