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

public:
    uint64  Get_PlayerId() const { return _playerId; }
    void    Set_PlayerId(uint64 id) { _playerId = id; }

private:
    uint64 _playerId = 0;

};

NS_END
