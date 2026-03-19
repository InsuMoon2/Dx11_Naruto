#include "pch.h"
#include "AnimNotify_Factory.h"

umap<string, AnimNotify_Factory::NotifyCreator> AnimNotify_Factory::_notifyCreators;
umap<string, AnimNotify_Factory::NotifyStateCreator> AnimNotify_Factory::_notifyStateCreators;

void AnimNotify_Factory::Initialize()
{
    _notifyCreators.clear();
    _notifyStateCreators.clear();
}

void AnimNotify_Factory::Register_Notify(const string& typeName, NotifyCreator creator)
{
    if (_notifyCreators.contains(typeName))
        return;

    _notifyCreators.emplace(typeName, creator);
}

void AnimNotify_Factory::Register_NotifyState(const string& typeName, NotifyStateCreator creator)
{
    if (_notifyStateCreators.contains(typeName))
        return;

    _notifyStateCreators.emplace(typeName, creator);
}

Shared<AnimNotify> AnimNotify_Factory::Create_Notify(const string& typeName)
{
    auto iter = _notifyCreators.find(typeName);
    if (iter == _notifyCreators.end())
        return nullptr;

    return iter->second();
}

Shared<AnimNotifyState> AnimNotify_Factory::Create_NotifyState(const string& typeName)
{
    auto iter = _notifyStateCreators.find(typeName);
    if (iter == _notifyStateCreators.end())
        return nullptr;

    return iter->second();
}

vector<string> AnimNotify_Factory::Get_NotifyTypeNames()
{
    vector<string> result;
    result.reserve(_notifyCreators.size()); // 크기 미리 할당

    for (const auto& [typeName, creator] : _notifyCreators)
    {
        result.push_back(typeName);
    }

    sort(result.begin(), result.end());

    return result;
}

vector<string> AnimNotify_Factory::Get_NotifyStateTypeNames()
{
    vector<string> result;
    result.reserve(_notifyStateCreators.size());

    for (const auto& [typeName, creator] : _notifyStateCreators)
    {
        result.push_back(typeName);
    }

    sort(result.begin(), result.end());

    return result;
}
