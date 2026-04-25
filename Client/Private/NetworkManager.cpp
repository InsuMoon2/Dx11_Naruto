#include "pch.h"
#include "NetworkManager.h"
#include "Service.h"
#include "ThreadManager.h"
#include "ServerSession.h"
#include "SocketUtils.h"
#include "Utils.h"

#include <fstream>

IMPLEMENT_SINGLETON(NetworkManager)

struct FNetworkConnectConfig
{
    // Target server IPv4 string used when the client creates the TCP service.
    wstring serverIp = TEXT("127.0.0.1");

    // Target server TCP port used when the client creates the TCP service.
    uint16 port = 7777;
};

// Converts JSON UTF-8/ASCII address text to the wide string required by NetAddress.
static wstring Convert_NetworkAddress_ToWide(const string& value)
{
    return wstring(value.begin(), value.end());
}

// Loads the client network endpoint before NetworkManager starts the ClientService.
static FNetworkConnectConfig Load_NetworkConnectConfig()
{
    FNetworkConnectConfig config{};

    // Runtime-editable endpoint file packaged with the client build.
    const fs::path configPath = fs::path("../../Client/Bin/Resources/Data/json/NetworkConfig.json");

    if (fs::exists(configPath) == false)
        return config;

    try
    {
        // Input stream for parsing the packaged network JSON file.
        ifstream file(configPath);

        // Parsed root object containing server_ip and port.
        const json root = json::parse(file);

        // IPv4 address text; defaults to localhost when the key is absent.
        const string serverIp = root.value("server_ip", "127.0.0.1");

        // TCP port; defaults to the current game server port when the key is absent.
        const uint16 port = static_cast<uint16>(root.value("port", 7777));

        config.serverIp = Convert_NetworkAddress_ToWide(serverIp);
        config.port = port;
    }
    catch (const std::exception& exception)
    {
        LOG_WARN("[Client] NetworkConfig.json load failed. Using default 127.0.0.1:7777. error={}", exception.what());
    }

    return config;
}

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
}

bool NetworkManager::Initialize()
{
    if (_service)
        return true;

    SocketUtils::Init();

    // Client connection endpoint loaded from packaged runtime config.
    const FNetworkConnectConfig connectConfig = Load_NetworkConnectConfig();

    // 클라이언트 서비스 생성
    _service = make_shared<ClientService>(
        NetAddress(connectConfig.serverIp, connectConfig.port),
        make_shared<IocpCore>(),// 서버 주소
        [this]() { return Create_Session(); }
    );


    if (_service->Start())
    {
        LOG_INFO("=== [Client] Network Service Started ===");
        LOG_INFO("[Client] Connecting to {}:{}...", Utils::ToString(connectConfig.serverIp), connectConfig.port);
        return true;
    }
    else
    {
        LOG_ERROR("[Client] Failed to start network service!");
        _session.reset();
        _service.reset();
        SocketUtils::Clear();
        return false;
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

bool NetworkManager::IsConnected() const
{
    return _session && _session->IsConnected();
}

void NetworkManager::Free()
{
    if (_service)
    {
        _service->CloseService();
    }

    _session.reset();
    _service.reset();

    SocketUtils::Clear();

    Base::Free();

}
