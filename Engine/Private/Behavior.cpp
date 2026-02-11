#include "pch.h"
#include "Behavior.h"

#include "BTNode.h"

//REGISTER_COMPONENT_FACTORY(Behavior, Protocol::COMPONENT_TYPE_AI)

Behavior::Behavior(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Behavior::Behavior(const Behavior& rhs)
    : Component(rhs)
    , _rootNode(rhs._rootNode)
    , _blackboard(rhs._blackboard)
{
}

Behavior::~Behavior()
{
}

HRESULT Behavior::Initialize_Prototype()
{

    return S_OK;
}

HRESULT Behavior::Initialize(void* arg)
{

    return S_OK;
}

void Behavior::Update(float timeDelta)
{
    if (_rootNode)
    {
        EBTNodeResult result = _rootNode->Update(timeDelta);

        if (result != EBTNodeResult::InProgress)
        {
            _rootNode->Initialize();
        }
    }
}

void Behavior::Set_RootNode(Shared<BTNode> rootNode)
{
    _rootNode = rootNode;

    if (_rootNode && _blackboard)
    {
        _rootNode->Set_Blackboard(_blackboard);
    }
}

void Behavior::Set_Blackboard(Shared<Blackboard> blackboard)
{
    _blackboard = blackboard;

    if (_rootNode)
    {
        _rootNode->Set_Blackboard(_blackboard);
    }
}

Shared<Behavior> Behavior::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Behavior>(device, context);

    instance->Initialize_Prototype();

    return instance;
}

Shared<Component> Behavior::Clone(void* arg)
{
    // TODO : 실제 게임에서는 BTNode를 Deep Copy 해야 함 (여기선 간단히 포인터 복사로 둠)
    auto instance = make_shared<Behavior>(*this);
    instance->Initialize(arg);

    return instance;
}
