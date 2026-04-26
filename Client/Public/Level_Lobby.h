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
    void    Update_PreviewCameraZoom();

    HRESULT Ready_PlayerNameplates();
    void    Update_PlayerNameplates();
    bool    Project_WorldToLobbyUI(const Vec3& worldPosition, Vec2& outUIPosition) const;
    void    Update_PreviewPlayerAnimations();

private:
    void Try_SendLobbyJoin();
    void Handle_LobbySnapshot(const Protocol::S_LobbySnapshot& pkt);

    void Handle_LobbyChat(const Protocol::S_LobbyChat& pkt);
    void Handle_LobbyStartGame();

    Shared<Player> Ensure_SlotPlayer(uint32 slot);

    void Apply_PlayerInfoToPreview(Shared<Player> player, const Protocol::ObjectInfo& info);
    void Refresh_PreviewPoseForFirstRender(Shared<Player> player);

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


private:
    struct FLobbyNamePlate
    {
        Shared<Background> background;
        wstring displayedName;
    };

    array<Shared<Player>, 2> _slotPlayers{};
    array<FLobbyNamePlate, 2> _slotNameplates{};
    array<bool, 2> _slotAppearancePlaying{};

private:
    Shared<Camera_Free> _previewCamera;
    Vec3 _previewLookTarget = Vec3(7.25f, 1.2f, 0.f);
    float _previewZoomStep = 0.35f;
    float _previewZoomMinDistance = 2.0f;
    float _previewZoomMaxDistance = 8.0f;
    Shared<Background> _chatBackground;
    Shared<Background> _chatInputBackground;

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
