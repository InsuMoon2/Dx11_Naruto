# Lobby + Chat Implementation Guide 1/2 - Protocol / Server

작성일: 2026-04-21

목표는 캐릭터 셋업 이후 `Lobby` 레벨로 이동하고, 로비에서 2명 프리뷰 + 경량 채팅 + 호스트 게임 시작을 처리하는 것이다.

핵심 원칙:

- 로비 입장에는 기존 `C_EnterGame`을 쓰지 않는다.
- `C_EnterGame`은 실제 GamePlay 입장 전용으로 유지한다.
- 로비는 `C_LobbyJoin`, `S_LobbySnapshot`, `C_LobbyChat`, `S_LobbyChat`, `C_LobbyStartGame`, `S_LobbyStartGame`으로 분리한다.
- 호스트는 1번 슬롯이다.

최종 흐름:

```text
CharacterSetup
  -> Lobby
      -> C_LobbyJoin
      -> S_LobbySnapshot
      -> C/S_LobbyChat
      -> C/S_LobbyStartGame
  -> Loading
  -> GamePlay
      -> 기존 C_EnterGame
```

---

## 1. Protocol.proto

파일:

```text
Server/Protobuf/Protocol/Protocol.proto
```

기존 메시지 아래에 추가한다.

```proto
message LobbyPlayerInfo
{
    uint64 lobby_id = 1;
    uint32 slot = 2;
    ObjectInfo info = 3;
    bool is_host = 4;
}

message C_LobbyJoin
{
    ObjectInfo info = 1;
}

message S_LobbySnapshot
{
    repeated LobbyPlayerInfo players = 1;
    uint64 my_lobby_id = 2;
}

message C_LobbyChat
{
    string message = 1;
}

message S_LobbyChat
{
    uint64 lobby_id = 1;
    string name = 2;
    string message = 3;
}

message C_LobbyStartGame
{
}

message S_LobbyStartGame
{
}
```

`my_lobby_id`를 넣는 이유:

- 클라가 본인이 호스트인지 정확히 알기 위해서다.
- 이름 비교로 호스트 판정하면 같은 이름이 들어왔을 때 꼬일 수 있다.

생성:

```text
Server/Protobuf/Protocol/GenProto.bat
```

확인:

```text
Engine/Public/Protocol.pb.h
Server/Protobuf/Bin/Protocol.pb.h
```

---

## 2. 서버 PacketID

파일:

```text
Server/GameServer/Public/Server_PacketHandler.h
```

enum에 추가한다.

```cpp
enum PacketID
{
    // Server -> Client
    S_Test           = 1,
    S_EnterGame      = 2,
    S_MyPlayer       = 3,
    S_AddObject      = 4,
    S_RemoveObject   = 5,
    S_Move           = 6,
    S_LobbySnapshot  = 7,
    S_LobbyChat      = 8,
    S_LobbyStartGame = 9,

    // Client -> Server
    C_EnterGame      = 49,
    C_Move           = 50,
    C_LobbyJoin      = 51,
    C_LobbyChat      = 52,
    C_LobbyStartGame = 53,
};
```

클래스 선언부에 추가한다.

```cpp
class Server_PacketHandler
{
public:
    static void HandlePacket(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    static void Handle_C_Move(shared_ptr<GameSession> session, BYTE* buffer, int32 len);
    static void Handle_C_EnterGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    // 로비 입장 요청을 받아 GameRoom의 로비 목록에 세션을 등록한다.
    static void Handle_C_LobbyJoin(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    // 로비 채팅 요청을 받아 같은 로비의 클라이언트들에게 브로드캐스트한다.
    static void Handle_C_LobbyChat(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    // 호스트의 게임 시작 요청을 받아 모든 로비 클라이언트에게 시작 패킷을 보낸다.
    static void Handle_C_LobbyStartGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    static SendBufferRef Make_S_MyPlayer(Protocol::ObjectInfo& info);
    static SendBufferRef Make_S_AddObject(Protocol::S_AddObject& pkt);
    static SendBufferRef Make_S_RemoveObject(Protocol::S_RemoveObject& pkt);
    static SendBufferRef Make_S_Move(Protocol::ObjectInfo& info);

    // 현재 로비 참가자 목록 전체를 클라이언트에게 보낸다.
    static SendBufferRef Make_S_LobbySnapshot(Protocol::S_LobbySnapshot& pkt);

    // 로비 채팅 한 줄을 클라이언트에게 보낸다.
    static SendBufferRef Make_S_LobbyChat(Protocol::S_LobbyChat& pkt);

    // 게임 시작 승인 패킷을 클라이언트에게 보낸다.
    static SendBufferRef Make_S_LobbyStartGame();
};
```

---

## 3. 서버 PacketHandler CPP

파일:

```text
Server/GameServer/Private/Server_PacketHandler.cpp
```

`HandlePacket()` 수정. 기존 `C_EnterGame`에는 `break`가 없으니 같이 넣는다.

```cpp
void Server_PacketHandler::HandlePacket(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    BufferReader reader(buffer, len);
    PacketHeader header;
    reader.Peek(&header);

    switch (header.id)
    {
    case C_Move:
        Handle_C_Move(session, buffer, len);
        break;

    case C_EnterGame:
        Handle_C_EnterGame(session, buffer, len);
        break;

    case C_LobbyJoin:
        Handle_C_LobbyJoin(session, buffer, len);
        break;

    case C_LobbyChat:
        Handle_C_LobbyChat(session, buffer, len);
        break;

    case C_LobbyStartGame:
        Handle_C_LobbyStartGame(session, buffer, len);
        break;

    default:
        break;
    }
}
```

추가 함수:

```cpp
void Server_PacketHandler::Handle_C_LobbyJoin(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyJoin pkt;
    ParsePacket(buffer, pkt);

    GRoom->Join_Lobby(session, pkt);
}

void Server_PacketHandler::Handle_C_LobbyChat(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyChat pkt;
    ParsePacket(buffer, pkt);

    GRoom->Handle_LobbyChat(session, pkt);
}

void Server_PacketHandler::Handle_C_LobbyStartGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyStartGame pkt;
    ParsePacket(buffer, pkt);

    GRoom->Handle_LobbyStartGame(session);
}

SendBufferRef Server_PacketHandler::Make_S_LobbySnapshot(Protocol::S_LobbySnapshot& pkt)
{
    return MakeSendBuffer(pkt, S_LobbySnapshot);
}

SendBufferRef Server_PacketHandler::Make_S_LobbyChat(Protocol::S_LobbyChat& pkt)
{
    return MakeSendBuffer(pkt, S_LobbyChat);
}

SendBufferRef Server_PacketHandler::Make_S_LobbyStartGame()
{
    Protocol::S_LobbyStartGame pkt;
    return MakeSendBuffer(pkt, S_LobbyStartGame);
}
```

---

## 4. GameRoom.h 로비 상태 추가

파일:

```text
Server/GameServer/Public/GameRoom.h
```

public 함수 추가:

```cpp
public: /* 로비 */
    // 캐릭터 셋업을 끝낸 클라이언트를 게임 시작 전 로비 명단에 등록한다.
    void Join_Lobby(Shared<GameSession> session, const Protocol::C_LobbyJoin& pkt);

    // 로비 채팅을 검증한 뒤 현재 로비 클라이언트들에게 브로드캐스트한다.
    void Handle_LobbyChat(Shared<GameSession> session, const Protocol::C_LobbyChat& pkt);

    // 1번 슬롯 호스트가 게임 시작을 요청했을 때 모든 로비 클라이언트에게 시작 패킷을 보낸다.
    void Handle_LobbyStartGame(Shared<GameSession> session);

    // 특정 세션에게 현재 로비 참가자 목록 전체를 보낸다.
    void Send_LobbySnapshot(Shared<GameSession> session);

    // 현재 로비 참가자 전체에게 참가자 목록을 다시 보낸다.
    void Broadcast_LobbySnapshot();
```

private 구조체/멤버 추가:

```cpp
private:
    struct FLobbyPlayer
    {
        // 로비에서만 사용하는 임시 id다. 실제 게임 objectId와 분리한다.
        uint64 lobbyId = 0;

        // 로비 표시 위치다. 1번 슬롯이 호스트다.
        uint32 slot = 0;

        // 이름과 커마 파츠를 다른 클라이언트에게 보여주기 위해 보관한다.
        Protocol::ObjectInfo info;

        // 로비 스냅샷/채팅/시작 패킷을 보내기 위한 세션 참조다.
        Weak<GameSession> session;
    };

    // 다음 로비 입장자에게 부여할 임시 id다.
    uint64 _nextLobbyId = 1;

    // 현재 로비에 들어와 있는 클라이언트 목록이다.
    map<uint64, FLobbyPlayer> _lobbyPlayers;
```

---

## 5. GameRoom.cpp 로비 구현

파일:

```text
Server/GameServer/Private/GameRoom.cpp
```

```cpp
void GameRoom::Join_Lobby(Shared<GameSession> session, const Protocol::C_LobbyJoin& pkt)
{
    CHECK_NULL(session);

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession == session)
        {
            lobbyPlayer.info = pkt.info();
            Broadcast_LobbySnapshot();
            return;
        }
    }

    if (_lobbyPlayers.size() >= 2)
        return;

    FLobbyPlayer lobbyPlayer{};
    lobbyPlayer.lobbyId = _nextLobbyId++;
    lobbyPlayer.slot = static_cast<uint32>(_lobbyPlayers.size()) + 1;
    lobbyPlayer.info = pkt.info();
    lobbyPlayer.session = session;

    _lobbyPlayers[lobbyPlayer.lobbyId] = lobbyPlayer;

    Broadcast_LobbySnapshot();
}

void GameRoom::Handle_LobbyChat(Shared<GameSession> session, const Protocol::C_LobbyChat& pkt)
{
    CHECK_NULL(session);

    if (pkt.message().empty())
        return;

    string message = pkt.message();
    if (message.size() > 80)
        message = message.substr(0, 80);

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession != session)
            continue;

        Protocol::S_LobbyChat chatPkt;
        chatPkt.set_lobby_id(lobbyId);
        chatPkt.set_name(lobbyPlayer.info.name().empty() ? "Player" : lobbyPlayer.info.name());
        chatPkt.set_message(message);

        Broadcast(Server_PacketHandler::Make_S_LobbyChat(chatPkt));
        return;
    }
}

void GameRoom::Handle_LobbyStartGame(Shared<GameSession> session)
{
    CHECK_NULL(session);

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession != session)
            continue;

        if (lobbyPlayer.slot != 1)
            return;

        Broadcast(Server_PacketHandler::Make_S_LobbyStartGame());
        return;
    }
}

void GameRoom::Send_LobbySnapshot(Shared<GameSession> session)
{
    CHECK_NULL(session);

    uint64 myLobbyId = 0;
    Protocol::S_LobbySnapshot pkt;

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto lobbySession = lobbyPlayer.session.lock();
        if (lobbySession == session)
            myLobbyId = lobbyId;

        auto* info = pkt.add_players();
        info->set_lobby_id(lobbyPlayer.lobbyId);
        info->set_slot(lobbyPlayer.slot);
        info->set_is_host(lobbyPlayer.slot == 1);
        *info->mutable_info() = lobbyPlayer.info;
    }

    pkt.set_my_lobby_id(myLobbyId);
    session->Send(Server_PacketHandler::Make_S_LobbySnapshot(pkt));
}

void GameRoom::Broadcast_LobbySnapshot()
{
    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto session = lobbyPlayer.session.lock();
        if (!session)
            continue;

        Send_LobbySnapshot(session);
    }
}
```

---

## 6. Leave_GameRoom 보강

`Leave_GameRoom()` 앞부분에 로비 세션 제거를 추가한다.

```cpp
void GameRoom::Leave_GameRoom(Shared<GameSession> session)
{
    if (session == nullptr)
        return;

    bool lobbyChanged = false;
    for (auto iter = _lobbyPlayers.begin(); iter != _lobbyPlayers.end(); )
    {
        auto lobbySession = iter->second.session.lock();
        if (lobbySession == session || !lobbySession)
        {
            iter = _lobbyPlayers.erase(iter);
            lobbyChanged = true;
            continue;
        }

        ++iter;
    }

    if (lobbyChanged)
        Broadcast_LobbySnapshot();

    Shared<Player> player = Find_Player(session->Get_PlayerId());
    if (player == nullptr)
        return;

    uint64 id = player->Get_ObjectID();
    Remove_Player(id);

    cout << "[Room] Player " << id
        << " Left (" << _players.size() << " players)" << endl;
}
```

---

## 7. 서버 쪽 확인 포인트

1. `C_LobbyJoin`은 `session->Set_PlayerId()`를 호출하지 않는다.
2. `C_LobbyJoin`은 `_players`에 `Player`를 만들지 않는다.
3. 실제 게임 입장만 `Enter_GameRoom()`에서 처리한다.
4. `S_LobbySnapshot`은 각 클라별로 `my_lobby_id`가 달라야 한다.
5. `C_LobbyStartGame`은 `slot == 1`만 통과한다.
6. 현재 구조는 최대 2인 로비 기준이다.

---

## 8. 1부 완료 후 해야 할 것

다음 문서에서 이어서 한다.

```text
Docs/Lobby_Chat_Implementation_Guide_2_Client.md
```

2부 내용:

- 클라 PacketID / Client_PacketHandler
- DelegateHub 이벤트
- Loader / Level_Loading
- CharacterSetup -> Lobby 이동
- Level_Lobby 헤더/CPP 구현 방향
