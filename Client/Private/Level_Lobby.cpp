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
#include "Level_Loading.h"

Level_Lobby::Level_Lobby(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level(device, context)
{
}

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

void Level_Lobby::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);
}

HRESULT Level_Lobby::Render()
{
    return Level::Render();
}

void Level_Lobby::On_CharInput(wchar_t ch)
{
    if (ch < 0x20)
        return;

    if (_chatInput.size() >= 80)
        return;

    _chatInput += ch;

    Refresh_ChatInputText();
}

HRESULT Level_Lobby::Ready_Layer_UI()
{
    return S_OK;
}

HRESULT Level_Lobby::Ready_PreviewScene()
{
    return S_OK;
}

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

void Level_Lobby::Handle_LobbyChat(const Protocol::S_LobbyChat& pkt)
{
    const wstring name = Utils::ToWString(pkt.name());
    const wstring message = Utils::ToWString(pkt.message());

    _chatLines.push_back(L"[" + name + L"] " + message);

    while (_chatLines.size() > 12)
        _chatLines.erase(_chatLines.begin());

    Refresh_ChatLogText();
}

void Level_Lobby::Handle_LobbyStartGame()
{
    GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, ELevelType::GamePlay, true, EGameplaySpawnMode::Server));
}

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

Shared<Level_Lobby> Level_Lobby::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Lobby>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Lobby");

        return nullptr;
    }

    return instance;
}

void Level_Lobby::Free()
{
    // 델리게이트 정리해야함
    GAME->Get_DelegateHub().OnLobbySnapshotReceived.Remove(_lobbySnapshotHandle);
    GAME->Get_DelegateHub().OnLobbyChatReceived.Remove(_lobbyChatHandle);
    GAME->Get_DelegateHub().OnLobbyStartGameReceived.Remove(_lobbyStartHandle);

    _lobbySnapshotHandle.Reset();
    _lobbyChatHandle.Reset();
    _lobbyStartHandle.Reset();

    Level::Free();
}
