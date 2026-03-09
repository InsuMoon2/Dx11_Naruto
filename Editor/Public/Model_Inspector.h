#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Model_Inspector : public Component_Inspector
{
public:
    explicit Model_Inspector() = default;
    virtual ~Model_Inspector() = default;

public:
    void Draw_Inspector(shared_ptr<Component> component) override;
    uint32 Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_MODEL; }
};

NS_END
