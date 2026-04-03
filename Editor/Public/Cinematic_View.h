#pragma once

#include "EditorWindow.h"
#include "CameraSequencerAdapter.h"

NS_BEGIN(Engine)
class RenderTarget;
class CameraTrack_Player;
class Camera_Cinematic;
class GameObject;
class Transform;
NS_END

NS_BEGIN(Editor)

class Cinematic_View : public EditorWindow
{
public:
    explicit Cinematic_View();
    virtual ~Cinematic_View() = default;

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;
    void Pre_Render() override;

    bool CanSave() const override;
    void Save() override;

public:
    void Create_NewSequence();
    void Load_Sequence(const string& path);
    void Save_Sequence(const string& path);

    // 키 프레임
    void Capture_KeyAtCurrentFrame();
    void Delete_SelectedKey();

    static fs::path Get_FolderPath()
    {
        return fs::path("../../Client/Bin/Resources/Data/json/Cinematics");
    }

private:
    void Draw_MenuBar();
    void Draw_PlayBar();

    void Draw_TopLayout();
    void Draw_Sequencer();
    void Draw_KeyList(float height);
    void Draw_PreviewPanel(float height);
    void Draw_KeyInspector(float height);
    void Draw_FilePopup();

    // 재생
    void Handle_PlaybackShortcut();
    void Apply_CurrentFrame();

    // 카메라
    void Ensure_PreviewCamera();

    void Sort_Keys();

    Shared<GameObject> Get_SelectedAnchorObject() const;
    Shared<Transform>  Get_SelectedAnchorTransform() const;

    void Sync_PreviewAnchor();
    bool Can_CaptureOwnerRelativeKey() const;

private:
    FCameraSequenceAsset        _asset;
    Shared<CameraTrack_Player>  _player;

    Shared<Camera_Cinematic>    _previewCamera;
    Shared<RenderTarget>        _previewRT;

    FSequencerUIState           _sequencerState;
    FCameraSequencerContext     _sequencerContext;
    CameraSequencerAdapter      _sequencerAdapter;

private:
    int32   _selectedKeyIndex = -1;
    bool    _isPlaying = false;
    bool    _isPreviewHovered = false;
    string  _currentFilePath;

    bool    _isOpenFilePopup = false;
    bool    _isSaveFilePopup = false;

    char    _saveFileNameBuf[128] = {};

    Matrix  _previewView = Matrix::Identity;
    Matrix  _previewProj = Matrix::Identity;

    ECinemaAnchorSpace _captureAnchorSpace = ECinemaAnchorSpace::WorldAbsolute;
    Weak<GameObject> _previewAnchorObject;

public:
    static Shared<Cinematic_View> Create();

};

NS_END
