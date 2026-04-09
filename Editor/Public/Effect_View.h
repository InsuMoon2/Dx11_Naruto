#pragma once

#include "EditorWindow.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)
class GameObject;
class RenderTarget;
class EffectComponent;
NS_END

NS_BEGIN(Editor)
class Editor_Camera_Free;
class Prefab_PreviewCameraSettings;

class Effect_View : public EditorWindow
{
private:
    enum class EPendingAction : uint8
    {
        None = 0,
        NewAsset, OpenAsset,
    };

public:
    explicit Effect_View();
    virtual ~Effect_View();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;
    void Pre_Render() override;
    bool CanSave() const override { return true; }
    void Save() override;

private:
    void Ensure_PreviewGridResources();
    void Draw_PreviewGrid() const;
    void Apply_PreviewCameraSettings();
    // 프리뷰가 검은 화면일 때도 카메라 상태를 직접 확인하고 복구할 수 있도록 우측 패널 상단에 그린다.
    void Draw_PreviewCameraInspector();

    void Draw_Header();
    void Draw_FilePopups();

    void Draw_Viewport();
    void Draw_LayerList();
    void Draw_Inspector();

    // 파일 세션 관련
    void New_EffectAsset();
    bool Load_EffectFile(const string& filePath);
    bool Save_CurrentFile();
    bool Save_AsEffect(const string& fileName);

    void Refresh_EffectFiles();
    void Request_NewAsset();
    void Request_OpenPopup();
    void Execute_PendingAction();
    void Clear_PendingAction();

    string Build_AssetDisplayName(const string& guid) const;

    bool Commit_AssetGuidChange(
        string& targetGuid,
        const string& newGuid,
        const char* expectedAssetType,
        const char* allowedRelativePrefix,
        bool& outResourceChanged);

    void Load_AssetPickerThumbnail(const string& guid);

    void Draw_AssetSlotPicker(
        const char* label,
        const char* popupId,
        const char* popupTitle,
        const char* dragPayloadType,
        const char* expectedAssetType,
        const char* allowedRelativePrefix,
        string& targetGuid,
        bool& outResourceChanged);

    void Draw_AssetSlotPopup(
        const char* popupId,
        const char* popupTitle,
        const char* expectedAssetType,
        const char* allowedRelativePrefix,
        string& targetGuid,
        bool& outResourceChanged);

    void Move_SelectedLayer(int32 direction);
    void Draw_ResolvedAssetInfo(const char* label, const string& guid) const;

    void Apply_SelectedLayerPreview(bool transformChanged, bool materialChanged, bool resourceChanged);

    void Restart_PreviewEffect();

private:
    Shared<GameObject> _previewObject;
    Shared<EffectComponent> _previewEffectCom;

private:
    Engine::FEffectAssetDesc _currentAsset;

    // 현재 열려있는 경로
    string _currentFilePath;
    vector<fs::path> _effectFiles;
    
    int _selectedLayerIdx = -1;

    bool _isPlaying = true;
    bool _showPreviewGrid = true;
    float _timeScale = 1.0f;
    // 이펙트 뷰 프리뷰에서만 메시를 흰색 불투명으로 강제 출력해 현재 메쉬 존재 여부를 바로 확인한다.
    bool _forceVisiblePreview = true;

    bool _showOpenFilePopup = false;
    bool _showSaveAsPopup = false;
    bool _showDirtyConfirmPopup = false;
    bool _requestFileNameFocus = false;

    EPendingAction _pendingAction = EPendingAction::None;
    string _pendingOpenPath;

    char _saveFileNameBuf[MAX_PATH] = {};
    char _assetPickerSearchBuf[128] = {};
    float _assetPickerThumbnailSize = 56.f;

    uset<string> _assetPickerNoThumbnailGuids;
    umap<string, ComPtr<ShaderResourceView>> _assetPickerThumbnailCache;

private:
    Shared<RenderTarget> _prevRT;
    Shared<Editor_Camera_Free> _previewCamera;
    Shared<Prefab_PreviewCameraSettings> _previewCameraSettings;

    Matrix _previewView;
    Matrix _previewProj;
    ImVec2 _previewScreenPos;
    ImVec2 _previewViewportSize;
    bool   _isPreviewHovered = false;

    // Grid Resources
    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _previewGridBatch;
    Shared<DirectX::BasicEffect>    _previewGridEffect;
    ComPtr<ID3D11InputLayout>       _previewGridInputLayout;
    ComPtr<ID3D11DepthStencilState> _previewGridDepthDisabledState;


public:
    static Shared<Effect_View> Create();

};

NS_END
