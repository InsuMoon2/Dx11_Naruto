#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class BTNode;

class ENGINE_DLL BTNode_Factory : public Base
{
    DECLARE_SINGLETON(BTNode_Factory)

public:
    using Creator = function<Shared<BTNode>()>;

    struct NodeInfo
    {
        string category;
        Creator creator;
    };

public:
    explicit BTNode_Factory() = default;
    virtual ~BTNode_Factory() = default;

public:
    void Initialize();
    void Register(const string& category, const string& typeName, Creator creator);

    // Engine 기본 노드
    void Register_EngineNodes();

    const umap<string, NodeInfo>& Get_RegisteredNodes() const { return _creators; }

private:
    umap<string, NodeInfo> _creators;

public:
    Shared<BTNode> Create(const string& typeName);
    void Free() override;
};
NS_END
