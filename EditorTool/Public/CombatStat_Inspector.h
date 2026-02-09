#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class CombatStat_Inspector : public Component_Inspector
{
public:
    explicit CombatStat_Inspector() = default;
    virtual ~CombatStat_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Engine::Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_COMBAT_STAT; }


};

NS_END
