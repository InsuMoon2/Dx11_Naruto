#pragma once

#include "Base.h"

class ClientService;

NS_BEGIN(Client)

class ServerSession;

class NetworkManager : public Base {
  DECLARE_SINGLETON(NetworkManager)

public:
  explicit NetworkManager();
  virtual ~NetworkManager();

public:
  void Initialize();
  void Update();

  shared_ptr<ServerSession> Create_Session();
  void Send_Packet(shared_ptr<SendBuffer> sendBuffer);

  shared_ptr<ServerSession> Get_Session();
  bool IsConnected() const;

  // TCP가 아니라, 온라인 접속 시도를 했나 판단
  bool IsNetworkEnabled() const { return _service != nullptr; }

private:
  shared_ptr<ClientService> _service;
  shared_ptr<ServerSession> _session;

public:
  virtual void Free() override;
};

NS_END
