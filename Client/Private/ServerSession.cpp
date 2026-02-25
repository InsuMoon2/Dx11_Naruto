#include "pch.h"
#include "ServerSession.h"
#include "Client_PacketHandler.h"

ServerSession::ServerSession()
{
}

ServerSession::~ServerSession()
{
}

void ServerSession::OnConnected()
{
    LOG_INFO("========================================");
    LOG_INFO("[Client] Connected to Server!");
    LOG_INFO("========================================");

    // 연결 직후 
}

void ServerSession::OnDisconnected()
{
    LOG_WARN("========================================");
    LOG_WARN("[Client] Disconnected from Server");
    LOG_WARN("========================================");
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    //LOG_INFO("========================================");
    //LOG_INFO("[Client] Received Packet!");
    //LOG_INFO("Size: {} bytes", len);
    //LOG_INFO("========================================");

    Client_PacketHandler::HandlePacket(static_pointer_cast<ServerSession>(shared_from_this()), buffer, len);
}

void ServerSession::OnSend(int32 len)
{
    
}
