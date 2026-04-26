#include "pch.h"
#include "Level_Lobby.h"
#include "Background.h"
#include "Camera_Free.h"
#include "Client_PacketHandler.h"
#include "Customizer_Manager.h"
#include "GameInstance.h"
#include "Level_CharacterSetup.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "Player.h"
#include "Spawn_Helper.h"
#include "UI_Text.h"
#include "Level_Loading.h"
#include "AnimationStateComponent.h"
#include "Model.h"
#include "PartObject.h"

static wstring Utf8ToWString(const string& value)
{
    if (value.empty())
        return {};

    const int32 convertedSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        nullptr,
        0);

    if (convertedSize <= 0)
        return {};

    wstring result(static_cast<size_t>(convertedSize), L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        result.data(),
        convertedSize);

    return result;
}

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

    Update_PreviewCameraZoom();
    Update_PlayerNameplates();
    Update_PreviewPlayerAnimations();

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
    const Vec2 viewport = { GAME->Get_UIReferenceWidth(), GAME->Get_UIReferenceHeight() };

    Background::FBackgroundDesc backDesc{};
    backDesc.name = TEXT("CharacterSetup_Background");
    backDesc.posX = viewport.x * 0.5f;
    backDesc.posY = viewport.y * 0.5f;
    backDesc.sizeX = viewport.x;
    backDesc.sizeY = viewport.y;

    backDesc.levelIndex = ETOI(ELevelType::Lobby);
    backDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    backDesc.textureIndex = ETOI(ECharacterSetupTexture::Lobby_Background);

    backDesc.zOrder = 0.5f;

    auto mainTitle = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &backDesc));

    CHECK_NULL(mainTitle, E_FAIL);
    mainTitle->Set_RenderGroup(ERenderGroup::BackgroundUI);

    Background::FBackgroundDesc chatChagDesc{};
    chatChagDesc.name = TEXT("ChattingChang");
    chatChagDesc.posX = viewport.x * 0.3f - 260.f;
    chatChagDesc.posY = viewport.y * 0.5f;
    chatChagDesc.sizeX = 450.f;
    chatChagDesc.sizeY = 656.f;

    chatChagDesc.levelIndex = ETOI(ELevelType::Lobby);
    chatChagDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    chatChagDesc.textureIndex = ETOI(ECharacterSetupTexture::ChattingChang);

    chatChagDesc.zOrder = 0.51f;

    _chatBackground = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &chatChagDesc));

    if (!_chatBackground)
        return E_FAIL;

    Background::FBackgroundDesc chatInputDesc{};
    chatInputDesc.name = TEXT("ChattingChang_Input");
    chatInputDesc.posX = chatChagDesc.posX - 26.f;
    chatInputDesc.posY = chatChagDesc.posY + 240.f;
    chatInputDesc.sizeX = 370.f;
    chatInputDesc.sizeY = 42.f;

    chatInputDesc.levelIndex = ETOI(ELevelType::Lobby);
    chatInputDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    chatInputDesc.textureIndex = ETOI(ECharacterSetupTexture::ChattingChang_Front);

    chatInputDesc.zOrder = 0.51f;

    _chatInputBackground = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &chatInputDesc));

    if (!_chatInputBackground)
        return E_FAIL;

    Background::FBackgroundDesc kunaiDesc{};
    kunaiDesc.name = TEXT("Roll_Selected");
    kunaiDesc.posX = 217.f;
    kunaiDesc.posY = 0.f;
    kunaiDesc.sizeX = 80.f;
    kunaiDesc.sizeY = 80.f;

    kunaiDesc.levelIndex = ETOI(ELevelType::Lobby);
    kunaiDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    kunaiDesc.textureIndex = ETOI(ECharacterSetupTexture::Roll_Selected);

    kunaiDesc.zOrder = 0.51f;

    auto kunai = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &kunaiDesc));

    kunai->Get_Transform()->Set_Parent(_chatInputBackground->Get_Transform());

    if (!kunai)
        return E_FAIL;

    {
        UI_Text::FUITextDesc chatLogDesc{};
        chatLogDesc.name = L"Lobby_Chat_Log";
        chatLogDesc.levelIndex = ETOI(ELevelType::Lobby);
        chatLogDesc.posX = 0.f;
        chatLogDesc.posY = -40.f;
        chatLogDesc.sizeX = 300.f;
        chatLogDesc.sizeY = 360.f;
        chatLogDesc.zOrder = 0.52f;
        chatLogDesc.text = L"[Debug] Chat Log Ready";
        chatLogDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
        chatLogDesc.style.fontSize = 18.f;
        chatLogDesc.style.color = Color(0.22f, 0.12f, 0.05f, 1.f);
        chatLogDesc.style.hAlign = ETextHAlign::Left;
        chatLogDesc.style.vAlign = ETextVAlign::Top;
        chatLogDesc.style.wordWrap = true;

        _chatLogText = static_pointer_cast<UI_Text>(
            GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &chatLogDesc));

        CHECK_NULL(_chatLogText, E_FAIL);

        _chatLogText->Get_Transform()->Set_Parent(_chatBackground->Get_Transform());
        CHECK_FAILED(GAME->Register_UI(EUILayer::HUD, _chatLogText), E_FAIL);
    }

    {
        UI_Text::FUITextDesc chatInputDesc{};
        chatInputDesc.name = L"Lobby_Chat_Input";
        chatInputDesc.levelIndex = ETOI(ELevelType::Lobby);
        chatInputDesc.posX = -85.f;
        chatInputDesc.posY = 0.f;
        chatInputDesc.sizeX = 140.f;
        chatInputDesc.sizeY = 40.f;
        chatInputDesc.zOrder = 0.53f;
        chatInputDesc.text = L"> _";
        chatInputDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
        chatInputDesc.style.fontSize = 18.f;
        chatInputDesc.style.color = Color(0.18f, 0.08f, 0.04f, 1.f);
        chatInputDesc.style.hAlign = ETextHAlign::Left;
        chatInputDesc.style.vAlign = ETextVAlign::Middle;
        chatInputDesc.style.wordWrap = false;

        _chatInputText = static_pointer_cast<UI_Text>(
            GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &chatInputDesc));

        CHECK_NULL(_chatInputText, E_FAIL);

        _chatInputText->Get_Transform()->Set_Parent(_chatInputBackground->Get_Transform());
        CHECK_FAILED(GAME->Register_UI(EUILayer::HUD, _chatInputText), E_FAIL);
    }

    {
        UI_Text::FUITextDesc startGuideDesc{};
        startGuideDesc.name = L"Lobby_Start_Guide";
        startGuideDesc.levelIndex = ETOI(ELevelType::Lobby);
        startGuideDesc.posX = 0.f;
        startGuideDesc.posY = -245.f;
        startGuideDesc.sizeX = 150.f;
        startGuideDesc.sizeY = 28.f;
        startGuideDesc.zOrder = 0.53f;
        startGuideDesc.text = L"Waiting for host...";
        startGuideDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
        startGuideDesc.style.fontSize = 15.f;
        startGuideDesc.style.color = Color(0.35f, 0.18f, 0.08f, 1.f);
        startGuideDesc.style.hAlign = ETextHAlign::Center;
        startGuideDesc.style.vAlign = ETextVAlign::Middle;
        startGuideDesc.style.wordWrap = false;

        _startGuideText = static_pointer_cast<UI_Text>(
            GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &startGuideDesc));

        CHECK_NULL(_startGuideText, E_FAIL);

        _startGuideText->Get_Transform()->Set_Parent(_chatBackground->Get_Transform());
        CHECK_FAILED(GAME->Register_UI(EUILayer::HUD, _startGuideText), E_FAIL);
    }

    CHECK_FAILED(Ready_PlayerNameplates(), E_FAIL);

    Refresh_ChatLogText();
    Refresh_ChatInputText();

    return S_OK;
}

HRESULT Level_Lobby::Ready_PreviewScene()
{
    GAME->Clear_Lights();

    {
        FLightDesc lightDesc{};
        lightDesc.type = ELightType::Directional;
        lightDesc.direction = Vec4(0.15f, -0.55f, -0.82f, 0.f);
        lightDesc.diffuse = Vec4(1.f, 0.96f, 0.9f, 1.f);
        lightDesc.ambient = Vec4(0.35f, 0.35f, 0.38f, 1.f);
        lightDesc.specular = Vec4(0.65f, 0.65f, 0.65f, 1.f);
        lightDesc.castShadow = false;

        CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);
    }

    {
        Camera_Free::FCameraFreeDesc cameraDesc{};
        cameraDesc.speedPerSec = 10.f;
        cameraDesc.mouseSensor = 0.1f;
        cameraDesc.eye = Vec3(7.5f, 1.45f, 2.35f);
        cameraDesc.at = _previewLookTarget;
        cameraDesc.fovY = XMConvertToRadians(60.f);
        cameraDesc.nearZ = 0.1f;
        cameraDesc.farZ = 1000.f;
        cameraDesc.scale = Vec3(1.f, 1.f, 1.f);

        _previewCamera = static_pointer_cast<Camera_Free>(
            GAME->Clone_And_Add_GameObject(
                ETOI(ELevelType::Static),
                Protocol::OBJECT_TYPE_CAMERA_FREE,
                ETOI(ELevelType::Lobby),
                TEXT("Layer_Camera"),
                &cameraDesc));

        CHECK_NULL(_previewCamera, E_FAIL);

        _previewCamera->Set_InputEnabled(false);
        GAME->Set_ActiveCamera(static_pointer_cast<Camera>(_previewCamera));
    }

    return S_OK;
}

void Level_Lobby::Update_PreviewCameraZoom()
{
    if (!_previewCamera)
        return;

    const float wheel = INPUT->GetMouseWheel();
    if (wheel == 0.f)
        return;

    auto transform = _previewCamera->Get_Transform();
    CHECK_NULL(transform);

    Vec3 cameraPos = transform->Get_LocalPosition();
    Vec3 toCamera = cameraPos - _previewLookTarget;

    float distance = toCamera.Length();
    if (distance <= FLT_EPSILON)
        return;

    toCamera.Normalize();

    distance = ::clamp(
        distance - wheel * _previewZoomStep,
        _previewZoomMinDistance,
        _previewZoomMaxDistance);

    cameraPos = _previewLookTarget + toCamera * distance;
    transform->Set_LocalPosition(cameraPos);
    transform->LookAt(_previewLookTarget);
}

HRESULT Level_Lobby::Ready_PlayerNameplates()
{
    for (uint32 i = 0; i < static_cast<uint32>(_slotNameplates.size()); ++i)
    {
        Background::FBackgroundDesc nameplateDesc{};
        nameplateDesc.name = format(L"Lobby_Player_Nameplate_{}", i + 1);
        nameplateDesc.levelIndex = ETOI(ELevelType::Lobby);
        nameplateDesc.posX = 0.f;
        nameplateDesc.posY = 0.f;
        nameplateDesc.sizeX = 260.f;
        nameplateDesc.sizeY = 42.f;
        nameplateDesc.zOrder = 0.56f;
        nameplateDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
        nameplateDesc.textureIndex = ETOI(ECharacterSetupTexture::SelectButton);

        nameplateDesc.textDesc.text = L" ";
        nameplateDesc.textDesc.offset = Vec2(0.f, 0.f);
        nameplateDesc.textDesc.size = Vec2(235.f, 34.f);
        nameplateDesc.textDesc.zOrderOffset = 0.01f;
        nameplateDesc.textDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
        nameplateDesc.textDesc.style.fontSize = 18.f;
        nameplateDesc.textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
        nameplateDesc.textDesc.style.hAlign = ETextHAlign::Center;
        nameplateDesc.textDesc.style.vAlign = ETextVAlign::Middle;
        nameplateDesc.textDesc.style.wordWrap = false;

        auto nameplate = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::HUD,
                &nameplateDesc));

        CHECK_NULL(nameplate, E_FAIL);

        nameplate->Set_LabelText(L"");
        nameplate->Set_Visibility(false);

        _slotNameplates[i].background = nameplate;
        _slotNameplates[i].displayedName.clear();
    }

    return S_OK;
}

void Level_Lobby::Update_PlayerNameplates()
{
    for (uint32 i = 0; i < static_cast<uint32>(_slotPlayers.size()); ++i)
    {
        auto player = _slotPlayers[i];
        auto& nameplate = _slotNameplates[i];

        if (!nameplate.background)
            continue;

        if (!player)
        {
            nameplate.background->Set_Visibility(false);
            continue;
        }

        Vec3 nameWorldPosition = player->Get_Transform()->Get_WorldPosition();
        nameWorldPosition.y += 2.15f;

        Vec2 uiPosition{};
        if (!Project_WorldToLobbyUI(nameWorldPosition, uiPosition))
        {
            nameplate.background->Set_Visibility(false);
            continue;
        }

        const wstring& playerName = player->Get_PlayerName();
        if (nameplate.displayedName != playerName)
        {
            nameplate.displayedName = playerName;
            nameplate.background->Set_LabelText(playerName);
        }

        nameplate.background->Set_UIPosition(uiPosition.x, 68.f);
        nameplate.background->Set_Visibility(true);
    }
}

bool Level_Lobby::Project_WorldToLobbyUI(const Vec3& worldPosition, Vec2& outUIPosition) const
{
    const float uiScale = GAME->Get_UIScale();
    if (uiScale <= FLT_EPSILON)
        return false;

    const Matrix* viewMatrix = GAME->Get_Transform(ETransformState::View);
    const Matrix* projMatrix = GAME->Get_Transform(ETransformState::Proj);

    if (!viewMatrix || !projMatrix)
        return false;

    Vec4 clipSpace = XMVector4Transform(
        Vec4(worldPosition.x, worldPosition.y, worldPosition.z, 1.f),
        (*viewMatrix) * (*projMatrix));

    if (clipSpace.w <= FLT_EPSILON)
        return false;

    const float invW = 1.f / clipSpace.w;
    const float ndcX = clipSpace.x * invW;
    const float ndcY = clipSpace.y * invW;

    if (ndcX < -1.f || ndcX > 1.f || ndcY < -1.f || ndcY > 1.f)
        return false;

    const float screenX = (ndcX * 0.5f + 0.5f) * GAME->Get_UIViewportWidth();
    const float screenY = (-ndcY * 0.5f + 0.5f) * GAME->Get_UIViewportHeight();

    const Vec2 uiOffset = GAME->Get_UIViewportOffset();

    outUIPosition.x = (screenX - uiOffset.x) / uiScale;
    outUIPosition.y = (screenY - uiOffset.y) / uiScale;

    return true;
}

void Level_Lobby::Update_PreviewPlayerAnimations()
{
    for (uint32 i = 0; i < static_cast<uint32>(_slotPlayers.size()); ++i)
    {
        if (!_slotAppearancePlaying[i])
            continue;

        auto player = _slotPlayers[i];
        if (!player)
        {
            _slotAppearancePlaying[i] = false;
            continue;
        }

        auto animState = player->Get_Component<AnimationStateComponent>();
        if (!animState)
        {
            _slotAppearancePlaying[i] = false;
            continue;
        }

        if (animState->Get_CurrentStateName() != "Appearance")
        {
            _slotAppearancePlaying[i] = false;
            continue;
        }

        if (animState->Is_CurrentStateFinished())
        {
            animState->Play_State("Lobby_Idle");
            _slotAppearancePlaying[i] = false;
        }
    }
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

    cout << "[LobbyClient] Snapshot myLobbyId=" << _myLobbyId
        << " players=" << pkt.players_size() << endl;

    for (int32 i = 0; i < pkt.players_size(); ++i)
    {
        const Protocol::LobbyPlayerInfo& playerInfo = pkt.players(i);
        const uint32 slot = playerInfo.slot();

        cout << "[LobbyClient] Player lobbyId=" << playerInfo.lobby_id()
            << " slot=" << slot
            << " host=" << (playerInfo.is_host() ? "true" : "false") << endl;

        if (playerInfo.lobby_id() == _myLobbyId && playerInfo.is_host())
            _isHost = true;

        if (slot == 0 || slot > _slotPlayers.size())
            continue;

        auto player = Ensure_SlotPlayer(slot);
        if (!player)
            continue;

        Apply_PlayerInfoToPreview(player, playerInfo.info());
    }

    if (_startGuideText)
        _startGuideText->Set_Text(_isHost ? L"F5 : 게임 시작" : L"호스트 ㄱㄷ");
}

void Level_Lobby::Handle_LobbyChat(const Protocol::S_LobbyChat& pkt)
{
    const wstring name = Utf8ToWString(pkt.name());
    const wstring message = Utf8ToWString(pkt.message());

    _chatLines.push_back(L"[" + name + L"] : " + message);

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
        ? Vec3(7.15f, 0.f, 0.f)
        : Vec3(5.65f, 0.f, 0.f);

    auto previewObj = Spawn_Helper::Prefab("PreviewPlayer")
        .AtLevel(ETOI(ELevelType::Lobby))
        .InLayer(TEXT("Layer_Player"))
        .Position(slotPosition)
        .Scale(Vec3(1.f, 1.f, 1.f))
        .Spawn();

    auto player = dynamic_pointer_cast<Player>(previewObj);
    if (!player)
        return nullptr;

    player->Get_Transform()->Set_LocalRotation(0.f, 0.f, 0.f);
    _slotPlayers[index] = player;

    if (auto animState = player->Get_Component<AnimationStateComponent>())
    {
        if (animState->Play_State("Appearance"))
            _slotAppearancePlaying[index] = true;
        else
            animState->Play_State("Lobby_Idle");

        Refresh_PreviewPoseForFirstRender(player);
    }

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

    Refresh_PreviewPoseForFirstRender(player);
}

void Level_Lobby::Refresh_PreviewPoseForFirstRender(Shared<Player> player)
{
    if (!player)
        return;

    if (auto model = player->Get_Component<Model>())
        model->Play_Animation(0.f, false);

    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        auto part = player->Get_PartObject(static_cast<ContainerObject::EPartSlot>(i));
        if (part)
            part->Update(0.f);
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

    GAME->Play_Sound(L"OutfitSelect_StartGame.wav", ESoundChannel::UI, 0.55f);

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
    GAME->Get_DelegateHub().OnLobbySnapshotReceived.Remove(_lobbySnapshotHandle);
    GAME->Get_DelegateHub().OnLobbyChatReceived.Remove(_lobbyChatHandle);
    GAME->Get_DelegateHub().OnLobbyStartGameReceived.Remove(_lobbyStartHandle);

    _lobbySnapshotHandle.Reset();
    _lobbyChatHandle.Reset();
    _lobbyStartHandle.Reset();

    Level::Free();
}
