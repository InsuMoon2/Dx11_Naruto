#include "pch.h"
#include "BTNode_Factory.h"

#include "BTComposite.h"
#include "BTRoot.h"
#include "BTTask_MoveTo.h"
#include "BTTask_Wait.h"

IMPLEMENT_SINGLETON(BTNode_Factory)

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
    auto& factory = GetInstance();

    // Root는 안보이게 (초기 생성 시 자동 생성)

    /* Composite */
    factory->Register("Hidden", "Root", []() {return make_shared<BTRoot>(); });
    factory->Register("Composite", "Sequence", []() {return make_shared<BTSequence>(); });
    factory->Register("Composite", "Selector", []() {return make_shared<BTSelector>(); });

    // TODO : Decorator (Invertor, Repeater) 추가 예정

    /* Task */
    factory->Register("Task", "Task_Wait", []() {return make_shared<BTTask_Wait>(); });
    factory->Register("Task", "Task_Move", []() {return make_shared<BTTask_MoveTo>(); });

}

Shared<BTNode> BTNode_Factory::Create(const string& typeName)
{
    auto it = _creators.find(typeName);

    if (it == _creators.end())
    {
        LOG_ERROR("BTNode not registered: {}", typeName);
        return nullptr;
    }

    return it->second.creator();
}

void BTNode_Factory::Free()
{
    _creators.clear();
    Base::Free();
}
