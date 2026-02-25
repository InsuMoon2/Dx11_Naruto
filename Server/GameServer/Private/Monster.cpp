#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
}

Monster::~Monster()
{
}

Shared<Monster> Monster::Create()
{
    auto monster = make_shared<Monster>();

    monster->info.set_objectid(s_idGenerator++);
    monster->info.set_objecttype(Protocol::OBJECT_TYPE_MONSTER);

    return monster;
}
