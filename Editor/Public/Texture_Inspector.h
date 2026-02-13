#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Texture_Inspector : public Component_Inspector
{
public:
    explicit Texture_Inspector() = default;
    virtual ~Texture_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT; }

};

NS_END
