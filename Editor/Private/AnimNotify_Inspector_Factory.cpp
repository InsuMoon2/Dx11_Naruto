#include "pch.h"
#include "AnimNotify_Inspector_Factory.h"
#include "AnimNotify_Factory.h"
#include "ANS_Test.h"
#include "AN_SpawnSkill_Inspector.h"
#include "AN_Test.h"

void AnimNotify_Inspector_Factory::Initialize()
{
    _notifyInspectors.clear();
    _notifyStateInspectors.clear();

    Register_Notifies();
    Register_NotifyState();
}

void AnimNotify_Inspector_Factory::Register_NotifyInspector(const string& typeName,
    Shared<AnimNotify_Inspector> inspector)
{
    _notifyInspectors[typeName] = inspector;
}

void AnimNotify_Inspector_Factory::Register_NotifyStateInspector(const string& typeName,
    Shared<AnimNotifyState_Inspector> inspector)
{
    _notifyStateInspectors[typeName] = inspector;
}

Shared<AnimNotify_Inspector> AnimNotify_Inspector_Factory::Get_NotifyInspector(const string& typeName)
{
    auto iter = _notifyInspectors.find(typeName);
    if (iter == _notifyInspectors.end())
        return nullptr;

    return iter->second;
}

Shared<AnimNotifyState_Inspector> AnimNotify_Inspector_Factory::Get_NotifyStateInspector(const string& typeName)
{
    auto iter = _notifyStateInspectors.find(typeName);
    if (iter == _notifyStateInspectors.end())
        return nullptr;

    return iter->second;
}

void AnimNotify_Inspector_Factory::Register_Notifies()
{
    Register_NotifyInspector("AN_SpawnSkill", make_shared<AN_SpawnSkill_Inspector>());
}

void AnimNotify_Inspector_Factory::Register_NotifyState()
{
    
}

Unique<AnimNotify_Inspector_Factory> AnimNotify_Inspector_Factory::Create()
{
    auto instance = make_unique<AnimNotify_Inspector_Factory>();

    instance->Initialize();

    return instance;
}
