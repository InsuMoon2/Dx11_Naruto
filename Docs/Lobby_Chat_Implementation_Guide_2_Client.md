# Lobby + Chat Implementation Guide 2/2 - Client / Level_Lobby

작성일: 2026-04-21

이 문서는 1부 서버 문서에 이어서 클라이언트 작업을 정리한다.

연결 문서:

```text
Docs/Lobby_Chat_Implementation_Guide_1_Server.md
```

현재 상태:

- `Client/Public/Client_Defines.h`에는 이미 `ELevelType::Lobby`가 들어가 있다.
- `Client/Public/Level_Lobby.h`, `Client/Private/Level_Lobby.cpp`는 만들어져 있다.
- 현재 `Level_Lobby`는 `Level_CharacterSetup` 복사본에 가깝다.
- 로비에서는 파츠 선택 UI를 쓰지 않고, 배경 + 왼쪽 채팅창 + 슬롯 플레이어 프리뷰만 쓴다.

---

## 1. 클라 PacketID / PacketHandler 선언

파일:

```text
Client/Public/Client_PacketHandler.h
```

enum에 추가한다.

```cpp
enum PacketID
{
    S_TEST           = 1,
    S_EnterGame      = 2,
    S_MyPlayer       = 3,
    S_AddObject      = 4,
    S_RemoveObject   = 5,
    S_Move           = 6,
    S_LobbySnapshot  = 7,
    S_LobbyChat      = 8,
    S_LobbyStartGame = 9,

    C_EnterGame      = 49,
    C_Move           = 50,
    C_LobbyJoin      = 51,
    C_LobbyChat      = 52,
    C_LobbyStartGame = 53,
};
```

클래스 선언부에 추가한다.

```cpp
class Client_PacketHandler
{
public:
    static void HandlePacket(Shared<ServerSession> session, BYTE* buffer, int32 len);

    static void Handle_S_TEST(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_MyPlayer(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_AddObject(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_RemoveObject(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_Move(Shared<ServerSession> session, BYTE* buffer, int32 len);

    // 로비 참가자 목록을 받아 현재 Lobby 레벨에 전달한다.
    static void Handle_S_LobbySnapshot(Shared<ServerSession> session, BYTE* buffer, int32 len);

    // 로비 채팅 메시지를 받아 현재 Lobby 레벨에 전달한다.
    static void Handle_S_LobbyChat(Shared<ServerSession> session, BYTE* buffer, int32 len);

    // 서버가 승인한 게임 시작 신호를 현재 Lobby 레벨에 전달한다.
    static void Handle_S_LobbyStartGame(Shared<ServerSession> session, BYTE* buffer, int32 len);

    static SendBufferRef Make_C_Move(const Protocol::ObjectInfo& objectInfo);
    static SendBufferRef Make_C_EnterGame(const Vec3& spawnPos, float rotY);

    // Customizer_Manager의 이름/커마를 로비 입장 패킷으로 만든다.
    static SendBufferRef Make_C_LobbyJoin();

    // 현재 입력된 채팅 문자열을 서버 로비 채팅 패킷으로 만든다.
    static SendBufferRef Make_C_LobbyChat(const wstring& message);

    // 호스트가 게임 시작을 요청할 때 서버로 보내는 패킷을 만든다.
    static SendBufferRef Make_C_LobbyStartGame();
};
```

---

## 2. DelegateHub 이벤트 추가

로비 패킷은 `Client_PacketHandler`에서 받지만, 실제 UI 갱신은 `Level_Lobby`가 해야 한다.  
그래서 `DelegateHub`로 패킷 결과를 넘긴다.

파일:

```text
Engine/Public/DelegateHub.h
```

include 추가:

```cpp
#include "Protocol.pb.h"
```

델리게이트 타입 추가:

```cpp
DECLARE_DELEGATE(FOnLobbySnapshotReceived, const Protocol::S_LobbySnapshot&);
DECLARE_DELEGATE(FOnLobbyChatReceived, const Protocol::S_LobbyChat&);
DECLARE_DELEGATE(FOnLobbyStartGameReceived);
```

멤버 추가:

```cpp
class ENGINE_DLL DelegateHub : public Base
{
public:
    // 서버에서 로비 참가자 목록을 받았을 때 현재 로비 레벨이 슬롯 프리뷰를 갱신하도록 호출된다.
    FOnLobbySnapshotReceived OnLobbySnapshotReceived;

    // 서버에서 로비 채팅을 받았을 때 현재 로비 레벨이 채팅 로그에 한 줄 추가하도록 호출된다.
    FOnLobbyChatReceived OnLobbyChatReceived;

    // 서버에서 게임 시작 승인을 받았을 때 현재 로비 레벨이 게임플레이 로딩으로 넘어가도록 호출된다.
    FOnLobbyStartGameReceived OnLobbyStartGameReceived;
};
```

---

## 3. Client_PacketHandler.cpp 구현

파일:

```text
Client/Private/Client_PacketHandler.cpp
```

필요 include 확인:

```cpp
#include "Customizer_Manager.h"
#include "NetworkManager.h"
```

`HandlePacket()` case 추가:

```cpp
case S_LobbySnapshot:
    Handle_S_LobbySnapshot(session, buffer, len);
    break;

case S_LobbyChat:
    Handle_S_LobbyChat(session, buffer, len);
    break;

case S_LobbyStartGame:
    Handle_S_LobbyStartGame(session, buffer, len);
    break;
```

수신 핸들러:

```cpp
void Client_PacketHandler::Handle_S_LobbySnapshot(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_LobbySnapshot pkt;
    ParsePacket(buffer, pkt);

    GAME->Get_DelegateHub().OnLobbySnapshotReceived.Broadcast(pkt);
}

void Client_PacketHandler::Handle_S_LobbyChat(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_LobbyChat pkt;
    ParsePacket(buffer, pkt);

    GAME->Get_DelegateHub().OnLobbyChatReceived.Broadcast(pkt);
}

void Client_PacketHandler::Handle_S_LobbyStartGame(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_LobbyStartGame pkt;
    ParsePacket(buffer, pkt);

    GAME->Get_DelegateHub().OnLobbyStartGameReceived.Broadcast();
}
```

송신 패킷:

```cpp
SendBufferRef Client_PacketHandler::Make_C_LobbyJoin()
{
    Protocol::C_LobbyJoin pkt;

    auto custom = GET_SINGLE(Customizer_Manager);
    auto* info = pkt.mutable_info();

    info->set_objecttype(Protocol::OBJECT_TYPE_PLAYER);
    info->set_name(Utils::ToString(custom->Get_PlayerName()));

    const auto& customDesc = custom->Get_CustomizerDesc();
    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        const wstring& partTag = customDesc.partTags[i];
        if (!partTag.empty())
        {
            (*info->mutable_equipparts())[i] = Utils::ToString(partTag);
        }
    }

    return MakeSendBuffer(pkt, C_LobbyJoin);
}

SendBufferRef Client_PacketHandler::Make_C_LobbyChat(const wstring& message)
{
    Protocol::C_LobbyChat pkt;

    wstring trimmed = message;
    if (trimmed.size() > 80)
        trimmed = trimmed.substr(0, 80);

    pkt.set_message(Utils::ToString(trimmed));
    return MakeSendBuffer(pkt, C_LobbyChat);
}

SendBufferRef Client_PacketHandler::Make_C_LobbyStartGame()
{
    Protocol::C_LobbyStartGame pkt;
    return MakeSendBuffer(pkt, C_LobbyStartGame);
}
```

---

## 4. Loading / Loader 연결

### 4.1. Level_Loading.cpp

파일:

```text
Client/Private/Level_Loading.cpp
```

include 추가:

```cpp
#include "Level_Lobby.h"
```

switch 추가:

```cpp
case ELevelType::Lobby:
    nextLevel = Level_Lobby::Create(_device, _context);
    break;
```

### 4.2. Loader.h

파일:

```text
Client/Public/Loader.h
```

선언 추가:

```cpp
private: /* Loading Level */
    HRESULT Loading_For_Maintitle();
    HRESULT Loading_For_GamePlay();
    HRESULT Loading_For_CharacterSetup();

    // 로비에서 사용할 배경/UI/프리뷰 플레이어 리소스를 준비한다.
    HRESULT Loading_For_Lobby();

    HRESULT Loading_For_Konoha();
```

### 4.3. Loader.cpp

파일:

```text
Client/Private/Loader.cpp
```

`Loading()` switch에 추가:

```cpp
case ELevelType::Lobby:
    hr = Loading_For_Lobby();
    break;
```

함수 추가:

```cpp
HRESULT Loader::Loading_For_Lobby()
{
    vector<FLoadJob> jobs;

    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        lstrcpy(_loadingText, TEXT("로비 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Model.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_SkillData.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);
    }

    {
        scoped_lock lock(_jobMutex);
        for (auto& job : jobs)
            _pendingJobs.push(std::move(job));
    }

    _totalJobs = static_cast<int32>(jobs.size());
    _completedJobs = 0;
    _prepareFinished = true;
    _isFinished = (_totalJobs.load() == 0);

    return S_OK;
}
```

---

## 5. CharacterSetup -> Lobby 이동

파일:

```text
Client/Private/Level_CharacterSetup.cpp
```

`Finish_CharacterSetup()` 마지막 이동지를 변경한다.

기존:

```cpp
GAME->Change_Level(
    ETOI(ELevelType::Loading),
    Level_Loading::Create(_device, _context, ELevelType::GamePlay, true, spawnMode));
```

변경:

```cpp
GAME->Change_Level(
    ETOI(ELevelType::Loading),
    Level_Loading::Create(_device, _context, ELevelType::Lobby, true, spawnMode));
```

이름/커마 저장 코드는 그대로 둔다.

```cpp
custom->Set_PlayerName(_pendingPlayerName);
```

---

## 6. Level_Lobby.h 정리 방향

파일:

```text
Client/Public/Level_Lobby.h
```

캐릭터 셋업에서 복사된 탭/옵션/이름 입력 관련 멤버는 제거한다.

남길 개념:

- 배경 UI
- 채팅 배경
- 채팅 로그 텍스트
- 채팅 입력 텍스트
- 게임 시작 안내 텍스트
- 1번/2번 슬롯 프리뷰 플레이어
- 로비 패킷 수신 델리게이트 핸들

헤더 예시:

```cpp
#pragma once

#include "Level.h"
#include "ContainerObject.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Background;
class Player;
class Camera_Free;

class Level_Lobby final : public Level
{
public:
    explicit Level_Lobby(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Lobby() = default;

public:
    HRESULT Initialize() override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    // 로비 채팅 입력을 받기 위해 윈도우 문자 입력 이벤트에서 호출된다.
    void On_CharInput(wchar_t ch) override;

private:
    // 로비 배경과 왼쪽 채팅창 UI를 생성한다.
    HRESULT Ready_Layer_UI();

    // 로비 슬롯에 표시할 프리뷰 카메라, 조명, 플레이어 자리만 준비한다.
    HRESULT Ready_PreviewScene();

    // 네트워크가 연결되면 한 번만 서버에 로비 입장을 요청한다.
    void Try_SendLobbyJoin();

    // 서버 스냅샷을 기준으로 1번/2번 슬롯 프리뷰 플레이어를 다시 맞춘다.
    void Handle_LobbySnapshot(const Protocol::S_LobbySnapshot& pkt);

    // 서버에서 받은 채팅 한 줄을 왼쪽 채팅 로그에 추가한다.
    void Handle_LobbyChat(const Protocol::S_LobbyChat& pkt);

    // 서버가 게임 시작을 승인하면 기존 게임플레이 로딩 흐름으로 이동한다.
    void Handle_LobbyStartGame();

    // 로비 슬롯 플레이어가 없으면 생성하고, 있으면 기존 객체를 반환한다.
    Shared<Player> Ensure_SlotPlayer(uint32 slot);

    // 서버에서 받은 이름/커마 정보를 프리뷰 플레이어에 적용한다.
    void Apply_PlayerInfoToPreview(Shared<Player> player, const Protocol::ObjectInfo& info);

    // 채팅 입력 상태를 UI_Text에 반영한다.
    void Refresh_ChatInputText();

    // 채팅 로그 배열을 하나의 문자열로 만들어 UI_Text에 반영한다.
    void Refresh_ChatLogText();

    // Enter 입력 시 현재 입력 중인 채팅을 서버로 보낸다.
    void Send_ChatInput();

    // 1번 슬롯 클라이언트만 서버에 게임 시작을 요청한다.
    void Request_StartGame();

private:
    // 로비 입장 패킷을 중복 전송하지 않기 위한 플래그다.
    bool _lobbyJoinSent = false;

    // 서버 스냅샷 기준 현재 클라이언트가 호스트인지 저장한다.
    bool _isHost = false;

    // 게임 시작 요청을 여러 번 보내지 않기 위한 플래그다.
    bool _startRequested = false;

    // 서버가 내려준 내 로비 id다.
    uint64 _myLobbyId = 0;

    // 로비 채팅 입력창에 현재 입력 중인 문자열이다.
    wstring _chatInput;

    // 왼쪽 채팅창에 표시할 최근 채팅 로그다.
    vector<wstring> _chatLines;

    // 1번/2번 슬롯에 표시할 로비 프리뷰 플레이어다.
    array<Shared<Player>, 2> _slotPlayers{};

    // 현재 로비 슬롯 플레이어를 비추는 프리뷰 카메라다.
    Shared<Camera_Free> _previewCamera;

    // 왼쪽 채팅창 배경이다.
    Shared<Background> _chatBackground;

    // 채팅 로그 본문 텍스트다.
    Shared<UI_Text> _chatLogText;

    // 현재 입력 중인 채팅 텍스트다.
    Shared<UI_Text> _chatInputText;

    // 호스트 여부와 시작 키 안내를 보여주는 텍스트다.
    Shared<UI_Text> _startGuideText;

    // 로비 스냅샷 델리게이트 해제를 위해 저장하는 핸들이다.
    FDelegateHandle _lobbySnapshotHandle;

    // 로비 채팅 델리게이트 해제를 위해 저장하는 핸들이다.
    FDelegateHandle _lobbyChatHandle;

    // 로비 게임 시작 델리게이트 해제를 위해 저장하는 핸들이다.
    FDelegateHandle _lobbyStartHandle;

public:
    static Shared<Level_Lobby> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
```

---

## 7. Level_Lobby.cpp 핵심 구현

파일:

```text
Client/Private/Level_Lobby.cpp
```

include:

```cpp
#include "pch.h"
#include "Level_Lobby.h"
#include "Background.h"
#include "Camera_Free.h"
#include "Client_PacketHandler.h"
#include "Customizer_Manager.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "Player.h"
#include "Spawn_Helper.h"
#include "UI_Text.h"
```

초기화:

```cpp
HRESULT Level_Lobby::Initialize()
{
    CHECK_FAILED(Ready_Layer_UI(), E_FAIL);
    CHECK_FAILED(Ready_PreviewScene(), E_FAIL);

    _lobbySnapshotHandle = GAME->Get_DelegateHub().OnLobbySnapshotReceived.Add(
        this, &Level_Lobby::Handle_LobbySnapshot);

    _lobbyChatHandle = GAME->Get_DelegateHub().OnLobbyChatReceived.Add(
        this, &Level_Lobby::Handle_LobbyChat);

    _lobbyStartHandle = GAME->Get_DelegateHub().OnLobbyStartGameReceived.Add(
        this, &Level_Lobby::Handle_LobbyStartGame);

    Refresh_ChatInputText();
    Refresh_ChatLogText();

    return S_OK;
}
```

업데이트:

```cpp
void Level_Lobby::Update(float timeDelta)
{
    Level::Update(timeDelta);

    Try_SendLobbyJoin();

    if (INPUT->KeyDown(KEY_TYPE::ENTER))
        Send_ChatInput();

    if (INPUT->KeyDown(KEY_TYPE::BACK) || INPUT->KeyPress(KEY_TYPE::BACK))
    {
        if (!_chatInput.empty())
        {
            _chatInput.pop_back();
            Refresh_ChatInputText();
        }
    }

    if (INPUT->KeyDown(KEY_TYPE::F5))
        Request_StartGame();
}
```

문자 입력:

```cpp
void Level_Lobby::On_CharInput(wchar_t ch)
{
    if (ch < 0x20)
        return;

    if (_chatInput.size() >= 80)
        return;

    _chatInput += ch;
    Refresh_ChatInputText();
}
```

로비 입장:

```cpp
void Level_Lobby::Try_SendLobbyJoin()
{
    if (_lobbyJoinSent)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    auto sendBuffer = Client_PacketHandler::Make_C_LobbyJoin();
    if (!sendBuffer)
        return;

    NetworkManager::GetInstance()->Send_Packet(sendBuffer);
    _lobbyJoinSent = true;
}
```

스냅샷 처리:

```cpp
void Level_Lobby::Handle_LobbySnapshot(const Protocol::S_LobbySnapshot& pkt)
{
    _myLobbyId = pkt.my_lobby_id();
    _isHost = false;

    for (int32 i = 0; i < pkt.players_size(); ++i)
    {
        const Protocol::LobbyPlayerInfo& playerInfo = pkt.players(i);
        const uint32 slot = playerInfo.slot();

        if (slot == 0 || slot > _slotPlayers.size())
            continue;

        auto player = Ensure_SlotPlayer(slot);
        if (!player)
            continue;

        Apply_PlayerInfoToPreview(player, playerInfo.info());

        if (playerInfo.lobby_id() == _myLobbyId && playerInfo.is_host())
            _isHost = true;
    }

    if (_startGuideText)
        _startGuideText->Set_Text(_isHost ? L"F5 : Game Start" : L"Waiting for host...");
}
```

채팅 처리:

```cpp
void Level_Lobby::Handle_LobbyChat(const Protocol::S_LobbyChat& pkt)
{
    const wstring name = Utils::ToWString(pkt.name());
    const wstring message = Utils::ToWString(pkt.message());

    _chatLines.push_back(L"[" + name + L"] " + message);

    while (_chatLines.size() > 12)
        _chatLines.erase(_chatLines.begin());

    Refresh_ChatLogText();
}
```

게임 시작 처리:

```cpp
void Level_Lobby::Handle_LobbyStartGame()
{
    GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, ELevelType::GamePlay, true, EGameplaySpawnMode::Server));
}
```

슬롯 플레이어:

```cpp
Shared<Player> Level_Lobby::Ensure_SlotPlayer(uint32 slot)
{
    if (slot == 0 || slot > _slotPlayers.size())
        return nullptr;

    const size_t index = static_cast<size_t>(slot - 1);
    if (_slotPlayers[index])
        return _slotPlayers[index];

    const Vec3 slotPosition = (slot == 1)
        ? Vec3(3.2f, 0.f, 0.f)
        : Vec3(5.8f, 0.f, 0.f);

    auto previewObj = Spawn_Helper::Prefab("PreviewPlayer")
        .AtLevel(ETOI(ELevelType::Lobby))
        .InLayer(TEXT("Layer_Player"))
        .Position(slotPosition)
        .Scale(Vec3(1.f, 1.f, 1.f))
        .Spawn();

    auto player = dynamic_pointer_cast<Player>(previewObj);
    if (!player)
        return nullptr;

    player->Get_Transform()->Set_LocalRotation(0.f, 180.f, 0.f);
    _slotPlayers[index] = player;

    return player;
}

void Level_Lobby::Apply_PlayerInfoToPreview(Shared<Player> player, const Protocol::ObjectInfo& info)
{
    if (!player)
        return;

    if (!info.name().empty())
        player->Set_PlayerName(Utils::ToWString(info.name()));

    for (auto& pair : info.equipparts())
    {
        ContainerObject::EPartSlot slot = static_cast<ContainerObject::EPartSlot>(pair.first);
        const wstring assetTag = Utils::ToWString(pair.second);
        player->Apply_CustomizingPart(slot, assetTag);
    }
}
```

채팅 전송/표시:

```cpp
void Level_Lobby::Refresh_ChatInputText()
{
    if (_chatInputText)
        _chatInputText->Set_Text(L"> " + _chatInput + L"_");
}

void Level_Lobby::Refresh_ChatLogText()
{
    if (!_chatLogText)
        return;

    wstring display;
    for (const auto& line : _chatLines)
    {
        display += line;
        display += L"\n";
    }

    _chatLogText->Set_Text(display);
}

void Level_Lobby::Send_ChatInput()
{
    if (_chatInput.empty())
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    auto sendBuffer = Client_PacketHandler::Make_C_LobbyChat(_chatInput);
    if (!sendBuffer)
        return;

    NetworkManager::GetInstance()->Send_Packet(sendBuffer);

    _chatInput.clear();
    Refresh_ChatInputText();
}
```

게임 시작 요청:

```cpp
void Level_Lobby::Request_StartGame()
{
    if (!_isHost || _startRequested)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    auto sendBuffer = Client_PacketHandler::Make_C_LobbyStartGame();
    if (!sendBuffer)
        return;

    NetworkManager::GetInstance()->Send_Packet(sendBuffer);
    _startRequested = true;
}
```

Free:

```cpp
void Level_Lobby::Free()
{
    GAME->Get_DelegateHub().OnLobbySnapshotReceived.Remove(_lobbySnapshotHandle);
    GAME->Get_DelegateHub().OnLobbyChatReceived.Remove(_lobbyChatHandle);
    GAME->Get_DelegateHub().OnLobbyStartGameReceived.Remove(_lobbyStartHandle);

    _lobbySnapshotHandle.Reset();
    _lobbyChatHandle.Reset();
    _lobbyStartHandle.Reset();

    Level::Free();
}
```

---

## 8. UI 생성 메모

`Ready_Layer_UI()`는 지금 네가 띄운 Background UI를 유지하고, 왼쪽에 `Background` 하나와 `UI_Text` 두 개만 추가하면 된다.

필요 UI:

```text
Lobby_Background
Lobby_Chat_Background
Lobby_Chat_Log
Lobby_Chat_Input
Lobby_Start_Guide
```

채팅창은 우선 기능 확인이 목적이므로 버튼 UI 없이 키보드로 처리한다.

```text
Enter = 채팅 전송
Backspace = 한 글자 삭제
F5 = 호스트 게임 시작
```

---

## 9. 테스트 체크리스트

1. `GenProto.bat` 실행 후 generated 파일 확인
2. Server 빌드
3. Client 빌드
4. 서버 실행
5. 1번 클라 캐릭터 셋업 완료
6. 1번 클라 로비 입장 확인
7. 2번 클라 캐릭터 셋업 완료
8. 양쪽 클라 로비에 2명 프리뷰 표시 확인
9. 1번 클라 채팅 -> 2번 클라 표시 확인
10. 2번 클라 채팅 -> 1번 클라 표시 확인
11. 2번 클라 F5 눌러도 시작 안 되는지 확인
12. 1번 클라 F5
13. 양쪽 클라 `Loading -> GamePlay` 이동 확인
14. GamePlay에서 기존 `C_EnterGame`, `S_MyPlayer`, `S_AddObject`, `S_Move` 정상 확인

---

## 10. 내일 바로 볼 포인트

가장 먼저 확인할 것:

```text
Client/Public/Level_Lobby.h
Client/Private/Level_Lobby.cpp
Client/Private/Client_PacketHandler.cpp
Engine/Public/DelegateHub.h
```

가장 위험한 실수:

```text
로비 입장에 C_EnterGame을 보내는 것
```

정답:

```text
C_LobbyJoin = 로비 표시용 이름/커마 등록
C_EnterGame = 실제 게임플레이 오브젝트 생성
```
