#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Inspector_Factory
{
public:
    explicit Inspector_Factory() = default;
    virtual ~Inspector_Factory() = default;

public:
    static void Initialize();
    static void Register_Inspector(uint32 typeId, shared_ptr<Component_Inspector> insepctor);
    static shared_ptr<Component_Inspector> Get_Insepctor(uint32 typeId);
    static bool Has_Insepctor(uint32 typeId);

private:
    static map<uint32, shared_ptr<Component_Inspector>> _inspectors;

};

NS_END
