#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class BTNode;

class ENGINE_DLL BTNode_Factory : public Base
{
    DECLARE_SINGLETON(BTNode_Factory)

public:
    using Creator = function<Shared<BTNode>()>;

public:
    explicit BTNode_Factory() = default;
    virtual ~BTNode_Factory() = default;

public:
    void Initialize();
    void Register(const string& typeName, Creator creator);

    // Engine 기본 노드
    void Register_EngineNodes();

private:
    umap<string, Creator> _creators;

public:
    Shared<BTNode> Create(const string& typeName);
    void Free() override;
};
NS_END
