#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Collider_Inspector : public Component_Inspector
{
public:
    explicit Collider_Inspector() = default;
    virtual ~Collider_Inspector() = default;

public:
    void    Draw_Inspector(Shared<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_COLLIDER; }
};

NS_END
