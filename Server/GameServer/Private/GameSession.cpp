#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "Server_PacketHandler.h"

GameSession::GameSession()
{


}

GameSession::~GameSession()
{
    cout << "~GameSeesion" << endl;
}

void GameSession::OnConnected()
{
    static atomic<uint64> s_idCounter = 1;
    _playerId = s_idCounter++;

    GSessionManager.Add(GetGameSessionRef());

    // 패킷 생성은 ServerPacket 핸들러에서
    Send(Server_PacketHandler::Make_S_MyPlayer(_playerId));
    GSessionManager.Broadcast(Server_PacketHandler::Make_S_AddObject(_playerId));

    cout << "Player " << _playerId << "입장" << endl;
}

void GameSession::OnDisconnected()
{
    GSessionManager.Remove(GetGameSessionRef());
    GSessionManager.Broadcast(Server_PacketHandler::Make_S_RemoveObject(_playerId));

    cout << "Player " << _playerId << " 퇴장" << endl;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    cout << "Recv Packet : " << len << "bytes" << endl;
    Server_PacketHandler::HandlePacket(GetGameSessionRef(), buffer, len);
}

void GameSession::OnSend(int32 len)
{
    PacketSession::OnSend(len);

    cout << "Send :  " << len << "bytes" << endl;
}

shared_ptr<GameSession> GameSession::GetGameSessionRef()
{
    return static_pointer_cast<GameSession>(shared_from_this());
}
