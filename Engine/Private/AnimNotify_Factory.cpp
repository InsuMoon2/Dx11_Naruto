#include "pch.h"
#include "AnimNotify_Factory.h"

map<wstring, AnimNotify_Factory::Creator> AnimNotify_Factory::_creators;

void AnimNotify_Factory::Initialize()
{
    _creators.clear();
}

void AnimNotify_Factory::Register(const wstring& name, Creator creator)
{
    if (_creators.contains(name))
        return;

    _creators.emplace(name, creator);
}

shared_ptr<AnimNotify> AnimNotify_Factory::Create(const wstring& name)
{
    auto iter = _creators.find(name);
    if (iter == _creators.end())
        return nullptr;

    return iter->second();
}
