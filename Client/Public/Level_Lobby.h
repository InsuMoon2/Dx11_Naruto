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

    void On_CharInput(wchar_t ch) override;

private:
    HRESULT Ready_Layer_UI();

    HRESULT Ready_PreviewScene();

    void Try_SendLobbyJoin();
    void Handle_LobbySnapshot(const Protocol::S_LobbySnapshot& pkt);

    void Handle_LobbyChat(const Protocol::S_LobbyChat& pkt);
    void Handle_LobbyStartGame();

    Shared<Player> Ensure_SlotPlayer(uint32 slot);

    void Apply_PlayerInfoToPreview(Shared<Player> player, const Protocol::ObjectInfo& info);

    void Refresh_ChatInputText();
    void Refresh_ChatLogText();

    void Send_ChatInput();
    void Request_StartGame();

private:
    bool _lobbyJoinSent = false;
    bool _isHost = false;
    bool _startRequested = false;

    uint64 _myLobbyId = 0;

    wstring _chatInput;

    vector<wstring> _chatLines;

    array<Shared<Player>, 2> _slotPlayers{};

    Shared<Camera_Free> _previewCamera;
    Shared<Background> _chatBackground;
    Shared<UI_Text> _chatLogText;

    Shared<UI_Text> _chatInputText;
    Shared<UI_Text> _startGuideText;

    FDelegateHandle _lobbySnapshotHandle;
    FDelegateHandle _lobbyChatHandle;
    FDelegateHandle _lobbyStartHandle;

public:
    static Shared<Level_Lobby> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
