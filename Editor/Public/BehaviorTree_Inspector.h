#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class BehaviorTree_Inspector : public Component_Inspector
{
public:
    explicit BehaviorTree_Inspector() = default;
    virtual ~BehaviorTree_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_AI; }

};

NS_END
