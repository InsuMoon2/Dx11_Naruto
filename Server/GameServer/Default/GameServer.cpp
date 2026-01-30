#include "pch.h"
#include "GameSession.h"
#include "Service.h"
#include "ThreadManager.h"

int main()
{
    SocketUtils::Init();

    shared_ptr<ServerService> service = make_shared<ServerService>(
        NetAddress(TEXT("127.0.0.1"), 7777),
        make_shared<IocpCore>(),
        []() { return make_shared<Server::GameSession>(); },
        100);

    assert(service->Start());

    cout << "=== Game Server Started on Port 7777 ===" << endl;

    while (true)
    {
        service->GetIocpCore()->Dispatch(0);
    }

    GThreadManager->Join();

    SocketUtils::Clear();
}


