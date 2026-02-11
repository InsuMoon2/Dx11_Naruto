#include "pch.h"
#include "NetworkManager.h"
#include "Service.h"
#include "ThreadManager.h"
#include "ServerSession.h"
#include "SocketUtils.h"

IMPLEMENT_SINGLETON(NetworkManager)

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::Initialize()
{
    SocketUtils::Init();

    // 클라이언트 서비스 생성
    _service = make_shared<ClientService>(
        NetAddress(TEXT("127.0.0.1"), 7777),
        make_shared<IocpCore>(),// 서버 주소
        [this]() { return Create_Session(); }
    );


    if (_service->Start())
    {
        // [수정] cout -> LOG_INFO
        LOG_INFO("=== [Client] Network Service Started ===");
        LOG_INFO("[Client] Connecting to 127.0.0.1:7777...");
    }
    else
    {
        // [수정] cout -> LOG_ERROR
        LOG_ERROR("[Client] Failed to start network service!");
    }

}

void NetworkManager::Update()
{
    if (_service)
    {
        // IOCP 이벤트 처리 (0ms = 논블로킹)
        _service->GetIocpCore()->Dispatch(0);
    }
}

shared_ptr<ServerSession> NetworkManager::Create_Session()
{
    _session = make_shared<ServerSession>();

    return _session;
}

void NetworkManager::Send_Packet(shared_ptr<SendBuffer> sendBuffer)
{
    if (_session)
        _session->Send(sendBuffer);
}

void NetworkManager::Free()
{
    if (_service)
    {
        _service->CloseService();
    }

    SocketUtils::Clear();

    Base::Free();

}
