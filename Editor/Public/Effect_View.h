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
    void Draw_Header();
    void Draw_Viewport();
    void Draw_LayerList();
    void Draw_Inspector();
    void Restart_PreviewEffect();

private:
    Engine::FEffectAssetDesc _currentAsset;

    string _savePath = "../../Client/Bin/Resources/Data/json/Effects/NewEffect.effect.json";
    int _selectedLayerIdx = -1;

    bool _isPlaying = true;
    bool _showPreviewGrid = true;
    float _timeScale = 1.0f;

    Shared<GameObject> _previewObject;
    Shared<EffectComponent> _previewEffectCom;

private:
    Shared<RenderTarget> _prevRT;
    Shared<Editor_Camera_Free> _previewCamera;
    Shared<Prefab_PreviewCameraSettings> _previewCameraSettings;

    Matrix _previewView;
    Matrix _previewProj;
    ImVec2 _previewScreenPos;
    ImVec2 _previewViewportSize;
    bool _isPreviewHovered = false;

    // Grid Resources
    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _previewGridBatch;
    Shared<DirectX::BasicEffect>    _previewGridEffect;
    ComPtr<ID3D11InputLayout>       _previewGridInputLayout;
    ComPtr<ID3D11DepthStencilState> _previewGridDepthDisabledState;


public:
    static Shared<Effect_View> Create();

};

NS_END
