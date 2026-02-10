#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Inspector_Factory
{
    DECLARE_SINGLETON(Inspector_Factory)

public:
    explicit Inspector_Factory() = default;
    virtual ~Inspector_Factory() = default;

public:
    void Initialize();
    void Register_Inspector(uint32 typeId, shared_ptr<Component_Inspector> insepctor);
    shared_ptr<Component_Inspector> Get_Inspector(uint32 typeId);
    bool Has_Inspector(uint32 typeId);

private:
    map<uint32, shared_ptr<Component_Inspector>> _inspectors;

};

NS_END
