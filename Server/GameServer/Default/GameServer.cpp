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

    // Bind address exposed to LAN clients during external PC connection tests.
    const wstring bindIp = TEXT("0.0.0.0");

    // Public game server TCP port used by the client NetworkConfig.json.
    const uint16 bindPort = 7777;

    shared_ptr<ServerService> service = make_shared<ServerService>(
        NetAddress(bindIp, bindPort),
        make_shared<IocpCore>(),
        []() { return make_shared<Server::GameSession>(); },
        100);

    // Release 빌드에서는 assert 인자가 실행되지 않으므로 서버 시작은 명시적으로 호출해야 한다.
    const bool isServiceStarted = service->Start();
    if (!isServiceStarted)
    {
        cerr << "Failed to start game server on 0.0.0.0:7777" << endl;
        return -1;
    }

    cout << "=== Game Server Started on 0.0.0.0:7777 ===" << endl;

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


