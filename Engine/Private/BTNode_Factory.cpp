#include "pch.h"
#include "BTNode_Factory.h"

#include "BTComposite.h"
#include "BTTask_MoveTo.h"
#include "BTTask_Wait.h"

IMPLEMENT_SINGLETON(BTNode_Factory)

void BTNode_Factory::Initialize()
{
    _creators.clear();
}

void BTNode_Factory::Register(const string& typeName, Creator creator)
{
    if (_creators.contains(typeName))
    {
        LOG_WARN("BTNode already registered: {}", typeName);
        return;
    }

    _creators.emplace(typeName, creator);

    LOG_INFO("Registered BTNode: {}", typeName);
}

void BTNode_Factory::Register_EngineNodes()
{
    auto& factory = GetInstance();

    factory->Register("Sequence", []() {return make_shared<BTSequence>(); });
    factory->Register("Selector", []() {return make_shared<BTSelector>(); });
    factory->Register("Task_Wait", []() {return make_shared<BTTask_Wait>(); });
    factory->Register("Task_MoveTo", []() {return make_shared<BTTask_MoveTo>(); });

}

Shared<BTNode> BTNode_Factory::Create(const string& typeName)
{
    auto it = _creators.find(typeName);

    if (it == _creators.end())
    {
        LOG_ERROR("BTNode not registered: {}", typeName);
        return nullptr;
    }

    return it->second();
}

void BTNode_Factory::Free()
{
    _creators.clear();
    Base::Free();
}
