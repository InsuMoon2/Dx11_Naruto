#include "pch.h"
#include "Player.h"

Player::Player()
{
}

Player::~Player()
{
}

Shared<Player> Player::Create()
{
    auto player = make_shared<Player>();

    player->info.set_objectid(s_idGenerator++);
    player->info.set_objecttype(Protocol::OBJECT_TYPE_PLAYER);

    return player;
}
