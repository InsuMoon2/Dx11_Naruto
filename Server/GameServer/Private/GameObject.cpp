#include "pch.h"
#include "GameObject.h"
#include "Player.h"
#include "Monster.h"
#include "Server_PacketHandler.h"
#include "GameRoom.h"

atomic<uint64> GameObject::s_idGenerator = 1;

void GameObject::Update()
{
}

void GameObject::BroadcastMove()
{
    if (room)
    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(info);
        room->Broadcast(sendBuffer);
    }
}
