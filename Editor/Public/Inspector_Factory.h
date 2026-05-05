#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Texture_Inspector;
class Model_Inspector;

class Inspector_Factory
{
    DECLARE_SINGLETON(Inspector_Factory)

public:
    explicit Inspector_Factory() = default;
    virtual ~Inspector_Factory() = default;

public:
    void Initialize();
    void Register_Inspector(uint32 typeId, shared_ptr<Component_Inspector> insepctor);
    bool Has_Inspector(uint32 typeId);

    Shared<Component_Inspector> Get_Inspector(uint32 typeId);
    Shared<Component_Inspector> Get_Inspector_ByType(Shared<Component> component);

private:
    map<uint32, Shared<Component_Inspector>> _inspectors;

    Shared<Texture_Inspector>   _textureInspector;
    Shared<Model_Inspector>     _modelInspector;
};

NS_END
