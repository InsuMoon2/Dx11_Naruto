#include "pch.h"
#include "NetworkManager.h"
#include "Service.h"
#include "ThreadManager.h"
#include "ServerSession.h"

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::Initialize()
{
}

void NetworkManager::Update()
{
}

shared_ptr<ServerSession> NetworkManager::Create_Session()
{
    return _session = make_shared<ServerSession>();
}

void NetworkManager::Send_Packet(shared_ptr<SendBuffer> sendBuffer)
{
    if (_session)
        _session->Send(sendBuffer);
}

void NetworkManager::Free()
{
    Base::Free();

}
