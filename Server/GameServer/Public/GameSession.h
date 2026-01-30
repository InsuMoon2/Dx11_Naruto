#pragma once

#include "Session.h"
#include "Server_Macro.h"

NS_BEGIN(Server)

class GameSession : public PacketSession
{
public:
    explicit GameSession();
    virtual ~GameSession();

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;

public:
    shared_ptr<GameSession> GetGameSessionRef();


};

NS_END
