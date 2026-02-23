#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class BTNode;

class ENGINE_DLL BTNode_Factory : public Base
{
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
    Shared<BTNode> Instantiate(const string& typeName);

private:
    umap<string, NodeInfo> _creators;

public:
    static Unique<BTNode_Factory> Create();
    void Free() override;
};
NS_END
