#include "pch.h"
#include "GameSession.h"

#include "GameRoom.h"
#include "Server_PacketHandler.h"

GameSession::GameSession()
{

}

GameSession::~GameSession()
{

}

void GameSession::OnConnected()
{
    GRoom->Enter_GameRoom(GetGameSessionRef());
}

void GameSession::OnDisconnected()
{
    GRoom->Leave_GameRoom(GetGameSessionRef());
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    Server_PacketHandler::HandlePacket(GetGameSessionRef(), buffer, len);
}

void GameSession::OnSend(int32 len)
{
    PacketSession::OnSend(len);

}

shared_ptr<GameSession> GameSession::GetGameSessionRef()
{
    return static_pointer_cast<GameSession>(shared_from_this());
}
