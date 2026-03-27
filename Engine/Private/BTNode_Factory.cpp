#include "pch.h"
#include "BTNode_Factory.h"

#include "BTComposite.h"
#include "BTRoot.h"
#include "BTTask_MoveTo.h"
#include "BTTask_SetAnimState.h"
#include "BTTask_Wait.h"

void BTNode_Factory::Initialize()
{
    _creators.clear();
}

void BTNode_Factory::Register(const string& category, const string& typeName, Creator creator)
{
    if (_creators.contains(typeName))
    {
        LOG_WARN("BTNode already registered: {}", typeName);
        return;
    }

    _creators[typeName] = { category, creator };

    LOG_INFO("Registered BTNode: [{}] {}", category, typeName);
}

void BTNode_Factory::Register_EngineNodes()
{
    // Root는 안보이게 (초기 생성 시 자동 생성)

    /* Composite */
    Register("Hidden", "Root", []() {return BTRoot::Create(); });
    Register("Composite", "Sequence", []() {return BTSequence::Create(); });
    Register("Composite", "Selector", []() {return BTSelector::Create(); });

    // TODO : Decorator (Invertor, Repeater) 추가 예정

    /* Task */
    Register("Task", "Task_Wait", []() {return BTTask_Wait::Create(); });
    Register("Task", "Task_Move", []() {return BTTask_MoveTo::Create(); });

    Register("Task", "Task_SetAnimState", []() { return BTTask_SetAnimState::Create(); });

}

Shared<BTNode> BTNode_Factory::Instantiate(const string& typeName)
{
    auto it = _creators.find(typeName);

    if (it == _creators.end())
    {
        LOG_ERROR("BTNode not registered: {}", typeName);
        return nullptr;
    }

    return it->second.creator();
}

Unique<BTNode_Factory> BTNode_Factory::Create()
{
    auto instance = make_unique<BTNode_Factory>();
    instance->Initialize();

    instance->Register_EngineNodes();

    return instance;
}

void BTNode_Factory::Free()
{
    _creators.clear();
    Base::Free();
}
