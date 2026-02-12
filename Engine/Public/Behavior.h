#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class BTNode;
class Blackboard;

class ENGINE_DLL Behavior : public Component
{
public:
    GENERATED_COMPONENT(Behavior, Protocol::COMPONENT_TYPE_AI)

public:
    explicit Behavior(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Behavior(const Behavior& rhs);
    virtual ~Behavior();

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* arg);
    virtual void    Update(float timeDelta);

public:
    void Set_RootNode(Shared<BTNode> rootNode);
    void Set_Blackboard(Shared<Blackboard> blackboard);

    Shared<Blackboard> Get_Blackboard() const { return _blackboard; }

    HRESULT Load_FromJson(const wstring& filePath);

private:
    Shared<BTNode> Create_Node(const string& typeName);

private:
    Shared<BTNode>     _rootNode;
    Shared<Blackboard> _blackboard;

public:
    static Shared<Behavior> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg = nullptr) override;

};

NS_END
