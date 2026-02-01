#include "pch.h"
#include "GameSession.h"

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
    cout << "Client 입장" << endl;

    vector<BuffData> dummy;
    auto sendBuffer = Server_PacketHandler::Make_S_TEST(0, 0, 0, dummy);
    Send(sendBuffer);
}

void GameSession::OnDisconnected()
{
    cout << "Client 탈주" << endl;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    cout << "Recv Packet : " << len << "bytes" << endl;

    // Echo back
    shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(len);
    ::memcpy(sendBuffer->Buffer(), buffer, len);
    sendBuffer->Close(len);

    Send(sendBuffer);
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
