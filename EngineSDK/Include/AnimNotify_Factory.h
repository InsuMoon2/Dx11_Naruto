#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class AnimNotify;
class AnimNotifyState;

class ENGINE_DLL AnimNotify_Factory : public Base
{
public:
    using NotifyCreator = function<Shared<AnimNotify>()>;
    using NotifyStateCreator = function<Shared<AnimNotifyState>()>;

public:
    void Initialize();

    static void Register_Notify(const string& typeName, NotifyCreator creator);
    static void Register_NotifyState(const string& typeName, NotifyStateCreator creator);

    static Shared<AnimNotify>       Create_Notify(const string& typeName);
    static Shared<AnimNotifyState>  Create_NotifyState(const string& typeName);

    static vector<string>           Get_NotifyTypeNames();
    static vector<string>           Get_NotifyStateTypeNames();

private:
    static umap<string, NotifyCreator> _notifyCreators;
    static umap<string, NotifyStateCreator> _notifyStateCreators;
};

// 매크로 정의
#define REGISTER_ANIM_NOTIFY(TYPE) \
    static struct AutoRegister_Notify_##TYPE { \
        AutoRegister_Notify_##TYPE() { \
            Engine::AnimNotify_Factory::Register_Notify(#TYPE, []() { return make_shared<TYPE>(); }); \
        } \
    } GAutoRegister_Notify_##TYPE;

#define REGISTER_ANIM_NOTIFY_STATE(TYPE) \
    static struct AutoRegister_NotifyState_##TYPE { \
        AutoRegister_NotifyState_##TYPE() { \
            Engine::AnimNotify_Factory::Register_NotifyState(#TYPE, []() { return make_shared<TYPE>(); }); \
        } \
    } GAutoRegister_NotifyState_##TYPE;

NS_END
