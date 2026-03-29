#include "pch.h"
#include "AnimNotify_Factory.h"


void AnimNotify_Factory::Register_Notify(const string& typeName, NotifyCreator creator)
{
    auto& creators = Get_NotifyCreators();

    if (creators.contains(typeName))
        return;

    creators.emplace(typeName, creator);
}

void AnimNotify_Factory::Register_NotifyState(const string& typeName, NotifyStateCreator creator)
{
    auto& creators = Get_NotifyStateCreators();

    if (creators.contains(typeName))
        return;

    creators.emplace(typeName, creator);
}

Shared<AnimNotify> AnimNotify_Factory::Create_Notify(const string& typeName)
{
    auto& creators = Get_NotifyCreators();

    auto iter = creators.find(typeName);
    if (iter == creators.end())
        return nullptr;

    return iter->second();
}

Shared<AnimNotifyState> AnimNotify_Factory::Create_NotifyState(const string& typeName)
{
    auto& creators = Get_NotifyStateCreators();

    auto iter = creators.find(typeName);
    if (iter == creators.end())
        return nullptr;

    return iter->second();
}

vector<string> AnimNotify_Factory::Get_NotifyTypeNames()
{
    auto& creators = Get_NotifyCreators();

    vector<string> result;
    result.reserve(creators.size());

    for (const auto& [typeName, creator] : creators)
        result.push_back(typeName);

    sort(result.begin(), result.end());

    return result;
}

vector<string> AnimNotify_Factory::Get_NotifyStateTypeNames()
{
    auto& creators = Get_NotifyStateCreators();

    vector<string> result;
    result.reserve(creators.size());

    for (const auto& [typeName, creator] : creators)
        result.push_back(typeName);

    sort(result.begin(), result.end());
    return result;
}

umap<string, AnimNotify_Factory::NotifyCreator>& AnimNotify_Factory::Get_NotifyCreators()
{
    static umap<string, NotifyCreator> creators;
    return creators;
}

umap<string, AnimNotify_Factory::NotifyStateCreator>& AnimNotify_Factory::Get_NotifyStateCreators()
{
    static umap<string, NotifyStateCreator> creators;
    return creators;
}
