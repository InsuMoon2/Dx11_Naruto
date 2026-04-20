#include "pch.h"

#include "GameRoom.h"
#include "GameSession.h"
#include "Service.h"
#include "ThreadManager.h"
#include <chrono>

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    SocketUtils::Init();

    shared_ptr<ServerService> service = make_shared<ServerService>(
        NetAddress(TEXT("127.0.0.1"), 7777),
        make_shared<IocpCore>(),
        []() { return make_shared<Server::GameSession>(); },
        100);

    assert(service->Start());

    cout << "=== Game Server Started on Port 7777 ===" << endl;

    // [추가] 서버 메인 루프의 실제 경과 시간을 측정해 방 로직에 넘기기 위한 이전 시각이다.
    auto previousTick = std::chrono::steady_clock::now();

    while (true)
    {
        service->GetIocpCore()->Dispatch(0);

        // [추가] 바쁜 루프에서도 몬스터 시뮬레이션 속도가 실제 시간과 맞게 흐르도록 frame delta를 계산한다.
        const auto currentTick = std::chrono::steady_clock::now();
        const float timeDelta = std::chrono::duration<float>(currentTick - previousTick).count();
        previousTick = currentTick;

        GRoom->Update(timeDelta);
    }

    GThreadManager->Join();

    SocketUtils::Clear();
}


