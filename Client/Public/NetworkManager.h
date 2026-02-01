#pragma once

#include "Base.h"

class ClientService;

NS_BEGIN(Client)

class ServerSession;

class NetworkManager : public Base
{
    DECLARE_SINGLETON(NetworkManager)

public:
    explicit NetworkManager();
    virtual ~NetworkManager();

public:
    void Initialize();
    void Update();

    shared_ptr<ServerSession> Create_Session();
    void Send_Packet(shared_ptr<SendBuffer> sendBuffer);

private:
    shared_ptr<ClientService> _service;
    shared_ptr<ServerSession> _session;

public:
    virtual void Free() override;
};

NS_END
