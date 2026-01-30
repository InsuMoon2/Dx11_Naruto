#pragma once

#include "Session.h"

NS_BEGIN(Client)

class ServerSession :public PacketSession
{
public:
    explicit ServerSession();
    virtual ~ServerSession();

public:
    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;

};

NS_END
