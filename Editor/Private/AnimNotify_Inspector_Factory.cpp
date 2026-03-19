#include "pch.h"
#include "AnimNotify_Inspector_Factory.h"
#include "AnimNotify_Factory.h"
#include "ANS_Test.h"
#include "AN_Test.h"

// 싱글톤일 필요가 있는가?
IMPLEMENT_SINGLETON(AnimNotify_Inspector_Factory)

void AnimNotify_Inspector_Factory::Initialize()
{
    _notifyInspectors.clear();
    _notifyStateInspectors.clear();

    // 커스텀 노티파이 등록
    {
        /* Notify */
        Register_Notifies();

        /* Notify State */
        Register_NotifyState();
    }
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
    AnimNotify_Factory::Register_Notify("AN_Test",
        []() -> Shared<AnimNotify> { return make_shared<AN_Test>(); });
}

void AnimNotify_Inspector_Factory::Register_NotifyState()
{
    AnimNotify_Factory::Register_NotifyState("ANS_Test",
        []() -> Shared<AnimNotifyState> { return make_shared<ANS_Test>(); });
}
