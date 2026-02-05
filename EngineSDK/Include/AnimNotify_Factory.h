#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class AnimNotify;

class ENGINE_DLL AnimNotify_Factory : public Base
{
public:
    using Creator = function<shared_ptr<AnimNotify>()>;

public:
    void Initialize();

    static void Register(const wstring& name, Creator creator);
    static shared_ptr<AnimNotify> Create(const wstring& name);

private:
    static map<wstring, Creator> _creators;
};

// 매크로 정의
#define REGISTER_ANIM_NOTIFY(TYPE) \
    static struct Helper_Notify_##TYPE { \
        Helper_Notify_##TYPE() { \
            Engine::AnimNotify_Factory::Register(TYPE::StaticClassName(), []() { return make_shared<TYPE>(); }); \
        } \
    } helper_notify_##TYPE;

NS_END
