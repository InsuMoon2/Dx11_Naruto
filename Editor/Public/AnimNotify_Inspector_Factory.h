#pragma once

#include "AnimNotify_Inspector.h"
#include "AnimNotifyState_Inspector.h"

NS_BEGIN(Editor)

class AnimNotify_Inspector_Factory
{
    DECLARE_SINGLETON(AnimNotify_Inspector_Factory)

public:
    AnimNotify_Inspector_Factory() = default;
    ~AnimNotify_Inspector_Factory() = default;

public:
    void Initialize();

    void Register_NotifyInspector(const string& typeName, Shared<AnimNotify_Inspector> inspector);
    void Register_NotifyStateInspector(const string& typeName, Shared<AnimNotifyState_Inspector> inspector);

    Shared<AnimNotify_Inspector>        Get_NotifyInspector(const string& typeName);
    Shared<AnimNotifyState_Inspector>   Get_NotifyStateInspector(const string& typeName);

private:
    umap<string, Shared<AnimNotify_Inspector>> _notifyInspectors;
    umap<string, Shared<AnimNotifyState_Inspector>> _notifyStateInspectors;
};

NS_END
