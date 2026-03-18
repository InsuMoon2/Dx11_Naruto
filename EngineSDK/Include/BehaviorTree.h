#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class BTNode;
class Blackboard;

class ENGINE_DLL BehaviorTree : public Component
{
public:
    GENERATED_COMPONENT(BehaviorTree, Protocol::COMPONENT_TYPE_BEHAVIOR)

public:
    explicit BehaviorTree(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit BehaviorTree(const BehaviorTree& rhs);
    virtual ~BehaviorTree();

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* arg);
    virtual void    BeginPlay() override;
    virtual void    Update(float timeDelta);

    virtual json    To_Json() const override;
    virtual void    From_Json(const json& data) override;

public:
    void Set_RootNode(Shared<BTNode> rootNode);
    void Set_Blackboard(Shared<Blackboard> blackboard);

    Shared<Blackboard> Get_Blackboard() const { return _blackboard; }

    HRESULT Load_FromJson(const wstring& filePath);

    map<int, EBTNodeResult> Get_AllNodeResults() const;


private:
    Shared<BTNode> Create_Node(const string& typeName);

private:
    Shared<BTNode>     _rootNode;
    Shared<Blackboard> _blackboard;

    string _btFilePath = "(None)";
    string _btGuid;

    bool _pendingInitialize = false;
    map<int, EBTNodeResult> _cachedNodeResults;

public:
    static Shared<BehaviorTree>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg = nullptr) override;

};

NS_END
