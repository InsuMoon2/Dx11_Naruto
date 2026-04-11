#include "pch.h"
#include "Effect_View.h"

#include "DebugDraw.h"
#include "EffectAsset_Serializer.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "EffectComponent.h"
#include "RenderTarget.h"
#include "Editor_Camera_Free.h"
#include "EffectPreviewRoot.h"
#include "Inspector.h"
#include "Prefab_PreviewCameraSettings.h"
#include "Reflection_Inspector.h"
#include "Texture.h"

#include "Notification_Manager.h"

static string Build_EffectAssetPickerLabel(const FAssetMeta& meta)
{
    const fs::path fullPath(meta.fullPath);
    const string fileName = fullPath.filename().string();
    const string relativePath = Utils::ToString(meta.relativePath);
    const string shortGuid = meta.guid.size() > 8 ? meta.guid.substr(0, 8) : meta.guid;

    string label = fileName;

    if (!meta.modelType.empty())
        label += "  [" + meta.modelType + "]";

    if (!relativePath.empty())
        label += "  {" + relativePath + "}";

    if (!shortGuid.empty())
        label += "  <" + shortGuid + ">";

    return label;
}

static vector<const FAssetMeta*> Get_EffectAssetsByTypeLoose(const string& expectedAssetType)
{
    vector<const FAssetMeta*> assets = GAME->Get_AssetByType(expectedAssetType);
    if (!assets.empty())
        return assets;

    if (!expectedAssetType.empty())
    {
        string fallback = expectedAssetType;
        fallback[0] = static_cast<char>(toupper(fallback[0]));
        assets = GAME->Get_AssetByType(fallback);
    }

    return assets;
}

static bool Is_EffectAssetTypeMatched(const FAssetMeta* meta, const string& expectedAssetType)
{
    if (!meta)
        return false;

    return Utils::ToLowerCopy(meta->type) == Utils::ToLowerCopy(expectedAssetType);
}

// Diffuse/Mask 피커에서 한 개가 아니라 여러 루트(예: Effects/;Skills/)를 동시에 허용할 수 있게 검사한다.
// allowedRelativePrefix는 ';' 구분 문자열로 들어오며, 어떤 prefix 하나라도 맞으면 통과시킨다.
static bool Is_EffectAssetInAllowedRelativeRoot(const FAssetMeta* meta, const char* allowedRelativePrefix)
{
    if (!meta)
        return false;

    if (allowedRelativePrefix == nullptr || allowedRelativePrefix[0] == '\0')
        return true;

    const string assetRelativePath = Utils::ToLowerCopy(fs::path(meta->relativePath).generic_string());
    const string allowedPrefixes = Utils::ToLowerCopy(string(allowedRelativePrefix));

    size_t begin = 0;
    while (begin <= allowedPrefixes.size())
    {
        const size_t end = allowedPrefixes.find(';', begin);
        string token = allowedPrefixes.substr(
            begin,
            end == string::npos ? string::npos : end - begin);

        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());

        if (!token.empty() && assetRelativePath.starts_with(token))
            return true;

        if (end == string::npos)
            break;

        begin = end + 1;
    }

    return false;
}

static string Build_EffectAssetPickerCacheKey(const char* popupId, const char* expectedAssetType, const char* allowedRelativePrefix)
{
    string key = popupId ? popupId : "";
    key += "|";
    key += expectedAssetType ? Utils::ToLowerCopy(string(expectedAssetType)) : "";
    key += "|";
    key += allowedRelativePrefix ? Utils::ToLowerCopy(string(allowedRelativePrefix)) : "";
    return key;
}

Effect_View::Effect_View()
    : EditorWindow(TEXT("Effect View"))
{
}

Effect_View::~Effect_View()
{
    if (_previewObject)
    {
        _previewObject->Set_Destroy(true);
    }

    GAME->Clear_Layers(ETOI(ELevelType::Prefab));
}

void Effect_View::Initialize()
{
    EditorWindow::Initialize();

    // 카메라 세팅
    {
        if (!_previewCameraSettings)
            _previewCameraSettings = Prefab_PreviewCameraSettings::Create();

        _previewCameraSettings->Load_Settings();
    }
    
    // 처음 에디터 킬 때 비활성화
    _isActive = false;

    _previewObject = EffectPreviewRoot::Create(GAME->Get_Device(), GAME->Get_Context());

    if (_previewObject)
    {
        GameObject::FGameObjectDesc goDesc{};
        goDesc.name = TEXT("EffectPreviewRoot");
        _previewObject->Initialize(&goDesc);

        _previewEffectCom = EffectComponent::Create(GAME->Get_Device(), GAME->Get_Context());
        if (_previewEffectCom)
        {
            _previewEffectCom->Set_ForceVisiblePreview(_forceVisiblePreview);
            _previewObject->Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _previewEffectCom);
            GAME->Add_GameObject(ETOI(ELevelType::Prefab), TEXT("Layer_Preview"), _previewObject);
        }
    }

    if (!_prevRT)
        _prevRT = RenderTarget::Create(GAME->Get_Device(), 600.f, 400.f);

    if (!_previewCamera)
    {
        _previewCamera = Editor_Camera_Free::Create(GAME->Get_Device(), GAME->Get_Context());
        auto desc = _previewCameraSettings->Build_Desc();

        _previewCamera->Initialize(&desc);
    }

    Apply_PreviewCameraSettings();
    Refresh_EffectFiles();
    New_EffectAsset();
}

void Effect_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    if (_isPlaying && _previewObject && _previewEffectCom)
    {
        float scaledDelta = timeDelta * _timeScale;
        _previewObject->Priority_Update(scaledDelta);
        _previewEffectCom->Update(scaledDelta);
    }
}

void Effect_View::OnGui()
{
    if (!_isActive) return;

    ImGui::Begin(Utils::ToString(_name).c_str(), &_isActive, ImGuiWindowFlags_NoScrollbar);

    Draw_Header();
    ImGui::Separator();

    if (ImGui::BeginTable("EffectLayout", 3, ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoDocking | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("LayerList", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Viewport", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableNextRow();

        // [좌측] 레이어 리스트
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("LayerListChild", ImVec2(0, 0), false);
        Draw_LayerList();

        ImGui::EndChild();

        // [중앙] 실시간 뷰포트 & 타임라인 컨테이너
        ImGui::TableSetColumnIndex(1);
        Draw_Viewport();

        // [우측] 프로퍼티 인스펙터
        ImGui::TableSetColumnIndex(2);
        ImGui::BeginChild("InspectorChild", ImVec2(0, 0), false);
        Draw_Inspector();

        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::End();
}

void Effect_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_isActive || !_previewObject|| !_prevRT || !_previewCamera) return;

    // 메인화면 카메라 백업
    Matrix savedView = *GAME->Get_Transform(ETransformState::View);
    Matrix savedProj = *GAME->Get_Transform(ETransformState::Proj);

    _previewView = _previewCamera->Get_ViewMatrix();
    float aspect = static_cast<float>(_prevRT->GetWidth()) / _prevRT->GetHeight();
    _previewProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.f);

    GAME->Set_Transform(ETransformState::View, _previewView);
    GAME->Set_Transform(ETransformState::Proj, _previewProj);

    FLightDesc savedLight{};
    bool hasSavedLight = false;
    if (const FLightDesc* lightDesc = GAME->Get_LightDesc(0))
    {
        savedLight = *lightDesc;
        hasSavedLight = true;
    }

    FLightDesc previewLight{};
    previewLight.direction = Vec4(0.f, -1.f, 0.f, 0.f);
    previewLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    previewLight.ambient = Vec4(0.5f, 0.5f, 0.5f, 1.f);

    GAME->Clear_Lights();
    GAME->Add_Light(previewLight);

    float dt = ImGui::GetIO().DeltaTime;
    GAME->Backup_RenderGroup();

    if (_isPlaying && _previewObject && _previewEffectCom)
    {
        _previewObject->Late_Update(dt * _timeScale);
        _previewEffectCom->Late_Update(dt * _timeScale);
    }
        

    _prevRT->Clear(Color(0.12f, 0.12f, 0.15f, 1.f));
    _prevRT->BindAsTarget();

    GAME->Draw(false, false);

    // GAME->Draw 이후 다른 경로에서 타깃이 바뀔 수 있어서 프리뷰 RT를 다시 명시적으로 물린다.
    _prevRT->BindAsTarget();

    Ensure_PreviewGridResources();
    Draw_PreviewGrid();

    GAME->BindBackBuffer();
    GAME->Restore_RenderGroup();
    GAME->Clear_Lights();

    if (hasSavedLight)
        GAME->Add_Light(savedLight);

    GAME->Set_Transform(ETransformState::View, savedView);
    GAME->Set_Transform(ETransformState::Proj, savedProj);
}

void Effect_View::Save()
{
    Save_CurrentFile();
}

void Effect_View::Ensure_PreviewGridResources()
{
    if (_previewGridBatch && _previewGridEffect && _previewGridInputLayout && _previewGridDepthDisabledState)
        return;

    _previewGridBatch = make_shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(GAME->Get_Context().Get());
    _previewGridEffect = make_shared<DirectX::BasicEffect>(GAME->Get_Device().Get());
    _previewGridEffect->SetVertexColorEnabled(true);

    const void* shaderByteCode = nullptr;
    size_t shaderByteCodeLength = 0;
    _previewGridEffect->GetVertexShaderBytecode(&shaderByteCode, &shaderByteCodeLength);

    if (FAILED(GAME->Get_Device()->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shaderByteCode,
        shaderByteCodeLength,
        &_previewGridInputLayout)))
    {
        _previewGridBatch.reset();
        _previewGridEffect.reset();
        _previewGridInputLayout.Reset();
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthDesc.StencilEnable = FALSE;

    if (FAILED(GAME->Get_Device()->CreateDepthStencilState(&depthDesc, _previewGridDepthDisabledState.GetAddressOf())))
    {
        _previewGridBatch.reset();
        _previewGridEffect.reset();
        _previewGridInputLayout.Reset();
        _previewGridDepthDisabledState.Reset();
    }
}

void Effect_View::Draw_PreviewGrid() const
{
    if (!_showPreviewGrid)
        return;

    if (!_previewGridBatch || !_previewGridEffect || !_previewGridInputLayout)
        return;

    auto context = GAME->Get_Context();
    if (!context)
        return;

    _previewGridEffect->SetWorld(Matrix::Identity);
    _previewGridEffect->SetView(_previewView);
    _previewGridEffect->SetProjection(_previewProj);

    context->IASetInputLayout(_previewGridInputLayout.Get());
    context->OMSetDepthStencilState(_previewGridDepthDisabledState.Get(), 0);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    _previewGridEffect->Apply(context.Get());

    _previewGridBatch->Begin();

    Vec3 gridCenter = Vec3::Zero;

    const XMVECTOR origin = XMVectorSet(gridCenter.x, gridCenter.y, gridCenter.z, 1.f);
    const XMVECTOR xAxis = XMVectorSet(20.f, 0.f, 0.f, 0.f);
    const XMVECTOR zAxis = XMVectorSet(0.f, 0.f, 20.f, 0.f);

    DX::DrawGrid(
        _previewGridBatch.get(),
        xAxis,
        zAxis,
        origin,
        40,
        40,
        XMVectorSet(0.65f, 0.65f, 0.7f, 1.f));

    DX::DrawRay(
        _previewGridBatch.get(),
        origin,
        XMVectorSet(20.f, 0.f, 0.f, 0.f),
        false,
        XMVectorSet(1.f, 0.2f, 0.2f, 1.f));

    DX::DrawRay(
        _previewGridBatch.get(),
        origin,
        XMVectorSet(0.f, 0.f, 20.f, 0.f),
        false,
        XMVectorSet(0.2f, 0.7f, 1.f, 1.f));

    _previewGridBatch->End();
}

void Effect_View::Apply_PreviewCameraSettings()
{
    if (!_previewCamera || !_previewCameraSettings)
        return;

    const auto desc = _previewCameraSettings->Build_Desc();

    _previewCamera->Apply_EditorDesc(desc);
}

// 프리뷰 오브젝트가 안 보여도 카메라 위치와 파라미터를 숫자로 확인하고 즉시 복구할 수 있게 한다.
void Effect_View::Draw_PreviewCameraInspector()
{
    if (!_previewCamera)
        return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.f, 1.f), "[ Preview Camera ]");
    ImGui::Separator();
    ImGui::Spacing();

    auto cameraTransform = _previewCamera->Get_Component<Transform>();
    if (cameraTransform)
    {
        ImGui::PushID("EffectPreviewCameraTransform");
        Inspector::Draw_Component(Transform::StaticTypeID(), cameraTransform);
        ImGui::PopID();
    }
    else
    {
        ImGui::TextDisabled("Preview camera transform not found.");
        ImGui::Spacing();
    }

    auto& cameraRefInfo = _previewCamera->Get_ReflectionInfo();
    if (!cameraRefInfo.properties.empty())
    {
        static Reflection_Inspector autoInspector;
        autoInspector.Draw_FromReflection(_previewCamera.get(), cameraRefInfo);
        ImGui::Spacing();
    }

    if (ImGui::Button("Save Current View", ImVec2(-1.f, 28.f)))
    {
        _previewCameraSettings->Capture_FromCamera(_previewCamera);
        _previewCameraSettings->Save_Settings();
    }

    if (ImGui::Button("Reset To Saved", ImVec2(-1.f, 28.f)))
    {
        Apply_PreviewCameraSettings();
    }

    if (ImGui::Button("Reset To Default", ImVec2(-1.f, 28.f)))
    {
        Prefab_PreviewCameraSettings defaultSettings;
        _previewCamera->Apply_EditorDesc(defaultSettings.Build_Desc());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void Effect_View::Draw_Header()
{
    if (ImGui::Button(ICON_FA_FILE_CIRCLE_PLUS " New", ImVec2(90.f, 0.f)))
    {
        Request_NewAsset();
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open", ImVec2(90.f, 0.f)))
    {
        Request_OpenPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save", ImVec2(90.f, 0.f)))
    {
        Save_CurrentFile();
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FILE_ARROW_DOWN " Save As", ImVec2(100.f, 0.f)))
    {
        string defaultName = !_currentFilePath.empty()
            ? fs::path(_currentFilePath).filename().string()
            : (_currentAsset.effectName.empty() ? "NewEffect.effect.json" : _currentAsset.effectName + ".effect.json");

        strcpy_s(_saveFileNameBuf, defaultName.c_str());
        _requestFileNameFocus = true;
        _showSaveAsPopup = true;
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_ROTATE " Restart", ImVec2(90.f, 0.f)))
    {
        Restart_PreviewEffect();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Preview Grid", &_showPreviewGrid);

    ImGui::SameLine();
    if (ImGui::Checkbox("Force Visible", &_forceVisiblePreview))
    {
        if (_previewEffectCom)
            _previewEffectCom->Set_ForceVisiblePreview(_forceVisiblePreview);
    }

    ImGui::SameLine();
    ImGui::TextDisabled("File: %s",
        _currentFilePath.empty()
        ? "<unsaved>"
        : fs::path(_currentFilePath).filename().string().c_str());

    if (IsDirty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), ICON_FA_TRIANGLE_EXCLAMATION " Unsaved");
    }

    Draw_FilePopups();
}

void Effect_View::Draw_FilePopups()
{
    if (_showOpenFilePopup)
    {
        ImGui::OpenPopup("Open Effect");
        _showOpenFilePopup = false;
    }

    if (ImGui::BeginPopupModal("Open Effect", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Select Effect File");
        ImGui::Separator();

        ImGui::BeginChild("EffectFileList", ImVec2(420.f, 240.f), true);
        for (const auto& path : _effectFiles)
        {
            string fileName = path.filename().string();

            if (ImGui::Selectable(fileName.c_str()))
            {
                if (IsDirty())
                {
                    _pendingAction = EPendingAction::OpenAsset;
                    _pendingOpenPath = path.string();
                    _showDirtyConfirmPopup = true;
                }
                else
                {
                    Load_EffectFile(path.string());
                }

                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (_showSaveAsPopup)
    {
        ImGui::OpenPopup("Save Effect As");
        _showSaveAsPopup = false;
    }

    if (ImGui::BeginPopupModal("Save Effect As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save Effect As");
        ImGui::Separator();

        ImGui::SetNextItemWidth(360.f);
        ImGui::InputText("File Name", _saveFileNameBuf, IM_ARRAYSIZE(_saveFileNameBuf));

        if (_requestFileNameFocus)
        {
            ImGui::SetKeyboardFocusHere(-1);
            _requestFileNameFocus = false;
        }

        if (ImGui::Button("Save", ImVec2(120.f, 0.f)))
        {
            if (strlen(_saveFileNameBuf) > 0)
            {
                if (Save_AsEffect(_saveFileNameBuf))
                    ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
        {
            Clear_PendingAction();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (_showDirtyConfirmPopup)
    {
        ImGui::OpenPopup("Unsaved Effect Changes");
        _showDirtyConfirmPopup = false;
    }

    if (ImGui::BeginPopupModal("Unsaved Effect Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Current effect has unsaved changes.");
        ImGui::Text("Do you want to save before continuing?");
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120.f, 0.f)))
        {
            if (_currentFilePath.empty())
            {
                string defaultName = _currentAsset.effectName.empty()
                    ? "NewEffect.effect.json"
                    : _currentAsset.effectName + ".effect.json";

                strcpy_s(_saveFileNameBuf, defaultName.c_str());
                _requestFileNameFocus = true;
                _showSaveAsPopup = true;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                if (Save_CurrentFile())
                    ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Discard", ImVec2(120.f, 0.f)))
        {
            Execute_PendingAction();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
        {
            Clear_PendingAction();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } 
}

void Effect_View::Draw_Viewport()
{
    const ImVec2 previewChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 45.f);
    ImGui::BeginChild("ViewportChild", previewChildSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        if (_prevRT)
        {
            const ImVec2 imagePos = ImGui::GetCursorScreenPos();
            const ImVec2 imageSize = ImGui::GetContentRegionAvail();

            // 뷰포트 그리기
            ImGui::Image((ImTextureID)_prevRT->Get_SRV(), imageSize);

            // 카메라 이동 제어
            _isPreviewHovered = ImGui::IsItemHovered();
            if (_isPreviewHovered && _previewCamera)
            {
                _previewCamera->Priority_Update(ImGui::GetIO().DeltaTime);
            }
        }
    }
    ImGui::EndChild();

    // 뷰포트 하단 타임라인/재생 바
    ImGui::BeginChild("TimelineControl", ImVec2(0, 0), false);
    ImGui::Separator();

    if (ImGui::Button(_isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY))
        _isPlaying = !_isPlaying;

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_STOP))
    {
        if (_previewEffectCom) _previewEffectCom->Stop_Effect();
        _isPlaying = false;
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(100.f);
    ImGui::SliderFloat("Speed", &_timeScale, 0.1f, 3.0f, "x%.1f");
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

void Effect_View::Draw_LayerList()
{
    ImGui::Text(ICON_FA_LAYER_GROUP " Emitters (Layers)");
    ImGui::Separator();
    ImGui::Spacing();

    for (int32 i = 0; i < static_cast<int32>(_currentAsset.layers.size()); ++i)
    {
        ImGui::PushID(i);

        auto& layer = _currentAsset.layers[i];
        string layerKindLabel = "Point";
        if (layer.base.kind == Engine::EEffectLayerKind::Mesh)
            layerKindLabel = "Mesh";
        else if (layer.base.kind == Engine::EEffectLayerKind::BillboardRect)
            layerKindLabel = "Billboard";

        string layerTitle = "[" + layerKindLabel + "] " + layer.base.layerName;

        if (ImGui::Selectable(layerTitle.c_str(), _selectedLayerIdx == i))
            _selectedLayerIdx = i;

        ImGui::PopID();
    }

    ImGui::Spacing();

    if (ImGui::Button("+ Add Mesh Emitter", ImVec2(-1.f, 30.f)))
    {
        Engine::FEffectLayerDesc newLayer{};
        newLayer.base.layerName = "NewMeshEmitter";
        newLayer.base.kind = Engine::EEffectLayerKind::Mesh;
        newLayer.base.localScale = Vec3(1.f, 1.f, 1.f);

        _currentAsset.layers.push_back(newLayer);
        _selectedLayerIdx = static_cast<int32>(_currentAsset.layers.size()) - 1;

        MarkDirty();
        Restart_PreviewEffect();
    }

    if (ImGui::Button("+ Add Point Emitter", ImVec2(-1.f, 30.f)))
    {
        Engine::FEffectLayerDesc newLayer{};
        newLayer.base.layerName = "NewPointEmitter";
        newLayer.base.kind = Engine::EEffectLayerKind::Point;
        newLayer.base.localScale = Vec3(1.f, 1.f, 1.f);
        newLayer.point.numInstances = 1;
        newLayer.point.scale = Vec2(0.25f, 0.25f);
        newLayer.point.speed = Vec2(0.f, 0.f);
        newLayer.point.lifeTime = Vec2(1.f, 1.f);
        newLayer.point.isLoop = true;
        newLayer.point.blendMode = Engine::EEffectBlendMode::Additive;
        newLayer.point.colorTint = Vec4(0.2f, 1.f, 1.f, 1.f);
        newLayer.point.opacity = 1.f;
        newLayer.point.moveMode = 2;

        _currentAsset.layers.push_back(newLayer);
        _selectedLayerIdx = static_cast<int32>(_currentAsset.layers.size()) - 1;

        MarkDirty();
        Restart_PreviewEffect();
    }

    if (ImGui::Button("+ Add Billboard Emitter", ImVec2(-1.f, 30.f)))
    {
        Engine::FEffectLayerDesc newLayer{};
        newLayer.base.layerName = "NewBillboardEmitter";
        newLayer.base.kind = Engine::EEffectLayerKind::BillboardRect;
        newLayer.base.localScale = Vec3(1.f, 1.f, 1.f);
        newLayer.billboard.blendMode = Engine::EEffectBlendMode::Additive;
        newLayer.billboard.baseTint = Vec4(0.55f, 0.92f, 1.f, 1.f);
        newLayer.billboard.ringTint = Vec4(0.85f, 1.f, 1.f, 1.f);
        newLayer.billboard.baseOpacity = 1.f;
        newLayer.billboard.ringOpacity = 0.35f;
        newLayer.billboard.useRing = true;
        newLayer.billboard.billboardToCamera = true;

        _currentAsset.layers.push_back(newLayer);
        _selectedLayerIdx = static_cast<int32>(_currentAsset.layers.size()) - 1;

        MarkDirty();
        Restart_PreviewEffect();
    }

    if (_selectedLayerIdx >= 0 && _selectedLayerIdx < static_cast<int32>(_currentAsset.layers.size()))
    {
        ImGui::Spacing();

        if (ImGui::Button(ICON_FA_ARROW_UP " Move Up", ImVec2(-1.f, 28.f)))
            Move_SelectedLayer(-1);

        if (ImGui::Button(ICON_FA_ARROW_DOWN " Move Down", ImVec2(-1.f, 28.f)))
            Move_SelectedLayer(1);

        if (ImGui::Button(ICON_FA_COPY " Duplicate Selected", ImVec2(-1.f, 30.f)))
        {
            FEffectLayerDesc clonedLayer = _currentAsset.layers[_selectedLayerIdx];

            clonedLayer.base.layerName += "_Copy"; 

            _currentAsset.layers.push_back(clonedLayer);

            _selectedLayerIdx = static_cast<int32>(_currentAsset.layers.size()) - 1;
            MarkDirty();
            Restart_PreviewEffect();
        }

        if (ImGui::Button(ICON_FA_TRASH " Delete Selected", ImVec2(-1.f, 30.f)))
        {
            _currentAsset.layers.erase(_currentAsset.layers.begin() + _selectedLayerIdx);

            if (_currentAsset.layers.empty())
                _selectedLayerIdx = -1;
            else
                _selectedLayerIdx = std::clamp(_selectedLayerIdx, 0, static_cast<int32>(_currentAsset.layers.size()) - 1);

            MarkDirty();
            Restart_PreviewEffect();
        }
    }
}

void Effect_View::Draw_Inspector()
{
    Draw_PreviewCameraInspector();

    if (_selectedLayerIdx < 0 || _selectedLayerIdx >= static_cast<int32>(_currentAsset.layers.size()))
    {
        ImGui::TextDisabled("No Emitter Selection");
        return;
    }

    auto& layer = _currentAsset.layers[_selectedLayerIdx];

    bool restartRequired = false;
    bool transformChanged = false;
    bool materialChanged = false;
    bool resourceChanged = false;

    char nameBuf[256] = {};
    strcpy_s(nameBuf, layer.base.layerName.c_str());
    if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf)))
    {
        layer.base.layerName = nameBuf;
        MarkDirty();
    }

    if (ImGui::Checkbox("Enabled", &layer.base.enabled))
    {
        MarkDirty();
        restartRequired = true;
    }

    if (ImGui::CollapsingHeader(ICON_FA_CLOCK " Transform & Time", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat("Duration", &layer.base.duration, 0.1f, -1.f, 100.f))
        {
            MarkDirty();
            restartRequired = true;
        }

        if (ImGui::DragFloat("Delay", &layer.base.startDelay, 0.05f))
        {
            MarkDirty();
            restartRequired = true;
        }

        if (ImGui::Checkbox("Loop", &layer.base.loop))
        {
            MarkDirty();
            restartRequired = true;
        }

        if (ImGui::DragFloat3("Local Pos", (float*)&layer.base.localPosition, 0.1f))
        {
            MarkDirty();
            transformChanged = true;
        }

        if (ImGui::DragFloat3("Local Rot", (float*)&layer.base.localRotation, 1.0f))
        {
            MarkDirty();
            transformChanged = true;
        }

        if (ImGui::DragFloat3("Local Scale", (float*)&layer.base.localScale, 0.05f))
        {
            MarkDirty();
            transformChanged = true;
        }

        if (ImGui::Checkbox("Scale Over Time", &layer.base.useScaleOverTime))
        {
            if (layer.base.useScaleOverTime)
            {
                layer.base.endScale = layer.base.localScale;

                if (layer.base.scaleDuration <= 0.f)
                    layer.base.scaleDuration = 1.0f;
            }

            MarkDirty();
            transformChanged = true;
        }

        if (layer.base.useScaleOverTime)
        {
            ImGui::Indent();

            if (ImGui::DragFloat("Scale Duration", &layer.base.scaleDuration, 0.01f, 0.01f, 10.f))
            {
                MarkDirty();
                transformChanged = true;
            }

            if (ImGui::DragFloat3("End Scale", (float*)&layer.base.endScale, 0.05f))
            {
                MarkDirty();
                transformChanged = true;
            }

            ImGui::Unindent();
        }

    }

    if (layer.base.kind == Engine::EEffectLayerKind::Mesh)
    {
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Draw_AssetSlotPicker(
                "Model",
                "##EffectModelPicker",
                ICON_FA_CUBE " Select Model",
                "CONTENT_MESH",
                "model",
                nullptr,
                layer.mesh.modelGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Diffuse",
                "##EffectDiffusePicker",
                ICON_FA_IMAGE " Select Diffuse Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.diffuseTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Emissive",
                "##EffectEmissivePicker",
                ICON_FA_IMAGE " Select Emissive Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.emissiveTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Emissive Gradation",
                "##EffectEmissiveGradationPicker",
                ICON_FA_IMAGE " Select Emissive Gradation Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.emissiveGradationTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Opacity",
                "##EffectOpacityPicker",
                ICON_FA_IMAGE " Select Opacity Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.opacityTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Opacity SubUV",
                "##EffectOpacitySubUvPicker",
                ICON_FA_IMAGE " Select Opacity SubUV Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.opacitySubUvTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Opacity Gradation",
                "##EffectOpacityGradationPicker",
                ICON_FA_IMAGE " Select Opacity Gradation Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.opacityGradationTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Legacy Mask",
                "##EffectMaskPicker",
                ICON_FA_IMAGE " Select Legacy Mask Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.maskTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "UV Distortion",
                "##EffectUvDistortionPicker",
                ICON_FA_IMAGE " Select UV Distortion Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.mesh.uvDistortionTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            int blend = static_cast<int>(layer.mesh.blendMode);
            if (ImGui::Combo("BlendMode", &blend, "Translucent\0Additive\0Opaque\0"))
            {
                layer.mesh.blendMode = static_cast<Engine::EEffectBlendMode>(blend);
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::Checkbox("Two Sided", &layer.mesh.twoSided))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::Checkbox("Use Opacity As Transparency", &layer.mesh.useOpacityAsTransparency))
            {
                MarkDirty();
                materialChanged = true;
            }
        }

        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Material Details", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::ColorEdit4("Color Tint", (float*)&layer.mesh.colorTint))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat2("UV Tiling", (float*)&layer.mesh.uvTiling, 0.05f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat2("UV Speed", (float*)&layer.mesh.uvScrollSpeed, 0.05f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat2("Distortion Strength", (float*)&layer.mesh.uvDistortionStrength, 0.005f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat2("Distortion Speed", (float*)&layer.mesh.uvDistortionSpeed, 0.05f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::SliderFloat("Opacity", &layer.mesh.opacity, 0.f, 1.f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat3("Rotation Axis", (float*)&layer.mesh.rotationAxis, 0.05f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat("Rotate Speed", &layer.mesh.rotationSpeed, 0.1f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat("Fresnel Power", &layer.mesh.fresnelPower, 0.1f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::DragFloat("Fresnel Mul", &layer.mesh.fresnelMultiplier, 0.1f))
            {
                MarkDirty();
                materialChanged = true;
            }
        }
    }
    else if (layer.base.kind == Engine::EEffectLayerKind::Point)
    {
        if (ImGui::CollapsingHeader(ICON_FA_IMAGE " Point Render", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Draw_AssetSlotPicker(
                "Texture",
                "##EffectPointTexturePicker",
                ICON_FA_IMAGE " Select Point Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.point.textureGuid,
                resourceChanged);

            ImGui::Spacing();

            int blend = static_cast<int>(layer.point.blendMode);
            if (ImGui::Combo("BlendMode", &blend, "Translucent\0Additive\0Opaque\0"))
            {
                layer.point.blendMode = static_cast<Engine::EEffectBlendMode>(blend);
                MarkDirty();
                resourceChanged = true;
            }

            int numInstances = static_cast<int>(layer.point.numInstances);
            if (ImGui::DragInt("Num Instances", &numInstances, 1.f, 1, 1000))
            {
                layer.point.numInstances = static_cast<uint32>((std::max)(1, numInstances));
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::Checkbox("Respawn Loop", &layer.point.isLoop))
            {
                MarkDirty();
                resourceChanged = true;
            }

            int moveMode = static_cast<int>(layer.point.moveMode);
            if (ImGui::Combo("Move Mode", &moveMode, "Drop\0Spread\0Static\0"))
            {
                layer.point.moveMode = static_cast<uint8>(moveMode);
                MarkDirty();
                resourceChanged = true;
            }
        }

        if (ImGui::CollapsingHeader(ICON_FA_WAND_MAGIC_SPARKLES " Point Details", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat3("Center", (float*)&layer.point.center, 0.05f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::DragFloat3("Range", (float*)&layer.point.range, 0.05f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::DragFloat2("Scale", (float*)&layer.point.scale, 0.01f, 0.001f, 10.f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::ColorEdit4("Color Tint", (float*)&layer.point.colorTint))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::SliderFloat("Opacity", &layer.point.opacity, 0.f, 1.f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::DragFloat2("Speed", (float*)&layer.point.speed, 0.01f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::DragFloat2("Life Time", (float*)&layer.point.lifeTime, 0.01f, 0.01f, 100.f))
            {
                MarkDirty();
                resourceChanged = true;
            }

            if (ImGui::DragFloat3("Pivot", (float*)&layer.point.pivot, 0.05f))
            {
                MarkDirty();
                resourceChanged = true;
            }
        }
    }
    else if (layer.base.kind == Engine::EEffectLayerKind::BillboardRect)
    {
        if (ImGui::CollapsingHeader(ICON_FA_IMAGE " Billboard Render", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Draw_AssetSlotPicker(
                "Base Texture",
                "##EffectBillboardBaseTexturePicker",
                ICON_FA_IMAGE " Select Billboard Base Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.billboard.baseTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            Draw_AssetSlotPicker(
                "Ring Texture",
                "##EffectBillboardRingTexturePicker",
                ICON_FA_IMAGE " Select Billboard Ring Texture",
                "CONTENT_TEXTURE",
                "texture",
                "Effects/;Skills/",
                layer.billboard.ringTextureGuid,
                resourceChanged);

            ImGui::Spacing();

            int blend = static_cast<int>(layer.billboard.blendMode);
            if (ImGui::Combo("BlendMode", &blend, "Translucent\0Additive\0Opaque\0"))
            {
                layer.billboard.blendMode = static_cast<Engine::EEffectBlendMode>(blend);
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::Checkbox("Use Ring", &layer.billboard.useRing))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::Checkbox("Billboard To Camera", &layer.billboard.billboardToCamera))
            {
                MarkDirty();
                materialChanged = true;
            }
        }

        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Billboard Details", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::ColorEdit4("Base Tint", (float*)&layer.billboard.baseTint))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::SliderFloat("Base Opacity", &layer.billboard.baseOpacity, 0.f, 1.f))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::ColorEdit4("Ring Tint", (float*)&layer.billboard.ringTint))
            {
                MarkDirty();
                materialChanged = true;
            }

            if (ImGui::SliderFloat("Ring Opacity", &layer.billboard.ringOpacity, 0.f, 1.f))
            {
                MarkDirty();
                materialChanged = true;
            }
        }
    }

    if (restartRequired)
    {
        Restart_PreviewEffect();
        return;
    }

    Apply_SelectedLayerPreview(transformChanged, materialChanged, resourceChanged);
}

void Effect_View::New_EffectAsset()
{
    _currentAsset = FEffectAssetDesc{};
    _currentAsset.effectName = "NewEffect";
    _currentAsset.autoPlay = true;
    _currentAsset.totalDuration = -1.f;

    _currentFilePath.clear();
    _pendingOpenPath.clear();
    _selectedLayerIdx = -1;
    _isPlaying = true;

    ClearDirty();
    Restart_PreviewEffect();
}

bool Effect_View::Load_EffectFile(const string& filePath)
{
    FEffectAssetDesc loadedAsset{};
    if (FAILED(EffectAsset_Serializer::Load_EffectAsset(filePath, loadedAsset)))
    {
        NOTIFY("이펙트 파일 로드 실패");
        return false;
    }

    _currentAsset = loadedAsset;
    _currentFilePath = filePath;
    _selectedLayerIdx = _currentAsset.layers.empty() ? -1 : 0;
    _isPlaying = true;

    ClearDirty();
    Restart_PreviewEffect();

    NOTIFY("이펙트 로드 완료");

    return true;
}

bool Effect_View::Save_CurrentFile()
{
    if (_currentFilePath.empty())
    {
        string defaultName = _currentAsset.effectName.empty()
            ? "NewEffect.effect.json"
            : _currentAsset.effectName + ".effect.json";

        strcpy_s(_saveFileNameBuf, defaultName.c_str());
        _requestFileNameFocus = true;
        _showSaveAsPopup = true;
        return false;
    }

    fs::path folder = Engine::EffectAsset_Serializer::Get_EffectFolderPath();
    if (!fs::exists(folder))
        fs::create_directories(folder);

    if (FAILED(Engine::EffectAsset_Serializer::Save_EffectAsset(_currentFilePath, _currentAsset)))
    {
        NOTIFY("이펙트 저장 실패");
        return false;
    }

    ClearDirty();
    Refresh_EffectFiles();
    NOTIFY("이펙트 저장");
    Execute_PendingAction();

    return true;
}

bool Effect_View::Save_AsEffect(const string& fileName)
{
    string normalizedName = fileName;
    if (normalizedName.empty())
        return false;

    if (!Utils::ToLowerCopy(normalizedName).ends_with(".effect.json"))
    {
        if (Utils::ToLowerCopy(normalizedName).ends_with(".json"))
            normalizedName = fs::path(normalizedName).stem().string() + ".effect.json";
        else
            normalizedName += ".effect.json";
    }

    fs::path folder = Engine::EffectAsset_Serializer::Get_EffectFolderPath();
    if (!fs::exists(folder))
        fs::create_directories(folder);

    fs::path savePath = folder / normalizedName;

    if (FAILED(Engine::EffectAsset_Serializer::Save_EffectAsset(savePath.string(), _currentAsset)))
    {
        NOTIFY("다른 이름으로 저장 실패");
        return false;
    }

    _currentFilePath = savePath.string();

    string pureName = savePath.stem().stem().string();
    if (!pureName.empty())
        _currentAsset.effectName = pureName;

    ClearDirty();
    Refresh_EffectFiles();
    NOTIFY("이펙트 저장 완료");
    Execute_PendingAction();

    return true;
}

void Effect_View::Refresh_EffectFiles()
{
    _effectFiles = Engine::EffectAsset_Serializer::Get_EffectFiles();
}

void Effect_View::Request_NewAsset()
{
    if (IsDirty())
    {
        _pendingAction = EPendingAction::NewAsset;
        _showDirtyConfirmPopup = true;
        return;
    }

    New_EffectAsset();
}

void Effect_View::Request_OpenPopup()
{
    Refresh_EffectFiles();
    _showOpenFilePopup = true;
}

void Effect_View::Execute_PendingAction()
{
    if (_pendingAction == EPendingAction::NewAsset)
    {
        New_EffectAsset();
    }
    else if (_pendingAction == EPendingAction::OpenAsset && !_pendingOpenPath.empty())
    {
        Load_EffectFile(_pendingOpenPath);
    }

    Clear_PendingAction();
}

void Effect_View::Clear_PendingAction()
{
    _pendingAction = EPendingAction::None;
    _pendingOpenPath.clear();
}

string Effect_View::Build_AssetDisplayName(const string& guid) const
{
    if (guid.empty())
        return "<empty>";

    const wstring resolvedPath = GAME->Resolve_AssetPath(guid);
    if (resolvedPath.empty())
        return "<invalid guid>";

    return fs::path(resolvedPath).filename().string();
}

bool Effect_View::Commit_AssetGuidChange(
    string& targetGuid,
    const string& newGuid,
    const char* expectedAssetType,
    const char* allowedRelativePrefix,
    bool& outResourceChanged)
{
    if (targetGuid == newGuid)
        return false;

    if (newGuid.empty())
    {
        targetGuid.clear();
        MarkDirty();
        outResourceChanged = true;
        return true;
    }

    const FAssetMeta* meta = GAME->Find_AssetByGUID(newGuid);
    if (!meta)
    {
        NOTIFY("유효하지 않은 GUID");
        return false;
    }

    if (!Is_EffectAssetTypeMatched(meta, expectedAssetType))
    {
        NOTIFY("지정 가능한 에셋 타입이 아닙니다");
        return false;
    }

    if (!Is_EffectAssetInAllowedRelativeRoot(meta, allowedRelativePrefix))
    {
        NOTIFY("이 슬롯에는 Effects 폴더 텍스처만 지정할 수 있습니다");
        return false;
    }

    targetGuid = newGuid;
    MarkDirty();
    outResourceChanged = true;
    return true;
}

void Effect_View::Load_AssetPickerThumbnail(const string& guid)
{
    if (guid.empty())
        return;

    if (_assetPickerThumbnailCache.contains(guid))
        return;

    if (_assetPickerNoThumbnailGuids.contains(guid))
        return;

    wstring absPath = GAME->Resolve_AssetPath(guid);
    bool isImage = false;
    if (!absPath.empty())
    {
        const wstring ext = Utils::ToWString(Utils::ToLowerCopy(fs::path(absPath).extension().string()));
        isImage = (ext == L".png" || ext == L".jpg" || ext == L".dds");
    }

    wstring loadPath;
    if (isImage)
    {
        loadPath = absPath;
    }
    else
    {
        const fs::path thumbPath = fs::path("../../Client/Bin/Resources/Thumbnails") / (guid + ".png");
        if (fs::exists(thumbPath))
            loadPath = thumbPath.wstring();
    }

    if (loadPath.empty())
    {
        _assetPickerNoThumbnailGuids.insert(guid);
        return;
    }

    auto texture = Texture::Create(GAME->Get_Device(), GAME->Get_Context(), loadPath, 1);
    if (texture && !texture->Get_SRVs().empty())
    {
        _assetPickerThumbnailCache[guid] = texture->Get_SRVs()[0];
        return;
    }

    _assetPickerNoThumbnailGuids.insert(guid);
}

void Effect_View::Draw_AssetSlotPicker(
    const char* label,
    const char* popupId,
    const char* popupTitle,
    const char* dragPayloadType,
    const char* expectedAssetType,
    const char* allowedRelativePrefix,
    string& targetGuid,
    bool& outResourceChanged)
{
    ImGui::PushID(label);

    ImGui::Text("%s", label);

    const string displayName = Build_AssetDisplayName(targetGuid);
    const float totalWidth = ImGui::GetContentRegionAvail().x;
    const float sideButtonWidth = 58.f;
    const float spacingWidth = ImGui::GetStyle().ItemSpacing.x;
    const float mainButtonWidth = (std::max)(120.f, totalWidth - (sideButtonWidth * 2.f + spacingWidth * 2.f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.26f, 0.26f, 1.f));

    if (ImGui::Button((displayName + "##AssetButton").c_str(), ImVec2(mainButtonWidth, 28.f)))
    {
        memset(_assetPickerSearchBuf, 0, sizeof(_assetPickerSearchBuf));
        ImGui::OpenPopup(popupId);
    }

    ImGui::PopStyleColor(2);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragPayloadType))
        {
            const string newGuid = static_cast<const char*>(payload->Data);
            Commit_AssetGuidChange(targetGuid, newGuid, expectedAssetType, allowedRelativePrefix, outResourceChanged);
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Drag & Drop or Click Select");
        ImGui::Separator();

        if (targetGuid.empty())
        {
            ImGui::TextDisabled("<empty>");
        }
        else
        {
            ImGui::Text("GUID: %s", targetGuid.c_str());

            const wstring resolvedPath = GAME->Resolve_AssetPath(targetGuid);
            if (!resolvedPath.empty())
                ImGui::TextDisabled("%s", Utils::ToString(resolvedPath).c_str());
            else
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Invalid GUID");
        }

        ImGui::EndTooltip();
    }

    ImGui::SameLine();

    if (ImGui::Button("Select", ImVec2(sideButtonWidth, 28.f)))
    {
        memset(_assetPickerSearchBuf, 0, sizeof(_assetPickerSearchBuf));
        ImGui::OpenPopup(popupId);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear", ImVec2(sideButtonWidth, 28.f)))
    {
        Commit_AssetGuidChange(targetGuid, "", expectedAssetType, allowedRelativePrefix, outResourceChanged);
    }

    Draw_AssetSlotPopup(
        popupId,
        popupTitle,
        expectedAssetType,
        allowedRelativePrefix,
        targetGuid,
        outResourceChanged);

    if (ImGui::TreeNodeEx("Advanced", ImGuiTreeNodeFlags_FramePadding))
    {
        char guidBuf[256] = {};
        strcpy_s(guidBuf, targetGuid.c_str());

        ImGui::InputText("Raw GUID", guidBuf, IM_ARRAYSIZE(guidBuf));
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            Commit_AssetGuidChange(targetGuid, guidBuf, expectedAssetType, allowedRelativePrefix, outResourceChanged);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void Effect_View::Draw_AssetSlotPopup(
    const char* popupId,
    const char* popupTitle,
    const char* expectedAssetType,
    const char* allowedRelativePrefix,
    string& targetGuid,
    bool& outResourceChanged)
{
    if (!ImGui::BeginPopup(popupId))
        return;

    ImGui::Text("%s", popupTitle);
    ImGui::Separator();

    ImGui::SetNextItemWidth(420.f);
    ImGui::InputTextWithHint("##EffectAssetSearch", "Search by file name...", _assetPickerSearchBuf, IM_ARRAYSIZE(_assetPickerSearchBuf));
    ImGui::Spacing();

    const string searchLower = Utils::ToLowerCopy(_assetPickerSearchBuf);
    const string expectedTypeLower = expectedAssetType ? Utils::ToLowerCopy(string(expectedAssetType)) : string{};
    const bool shouldShowResults = (expectedTypeLower != "texture") || !searchLower.empty();
    const string cacheKey = Build_EffectAssetPickerCacheKey(popupId, expectedAssetType, allowedRelativePrefix);

    static string s_cachedPickerKey;
    static string s_cachedSearch;
    static vector<const FAssetMeta*> s_sortedAssets;
    static vector<const FAssetMeta*> s_filteredAssets;

    if (s_cachedPickerKey != cacheKey)
    {
        s_cachedPickerKey = cacheKey;
        s_cachedSearch.clear();
        s_sortedAssets = Get_EffectAssetsByTypeLoose(expectedAssetType);
        s_filteredAssets.clear();

        sort(s_sortedAssets.begin(), s_sortedAssets.end(), [](const FAssetMeta* lhs, const FAssetMeta* rhs)
        {
            if (!lhs || !rhs)
                return lhs != nullptr;

            const string lhsName = Utils::ToLowerCopy(fs::path(lhs->fullPath).filename().string());
            const string rhsName = Utils::ToLowerCopy(fs::path(rhs->fullPath).filename().string());

            if (lhsName != rhsName)
                return lhsName < rhsName;

            return Utils::ToLowerCopy(Utils::ToString(lhs->relativePath)) < Utils::ToLowerCopy(Utils::ToString(rhs->relativePath));
        });
    }

    if (s_cachedSearch != searchLower)
    {
        s_cachedSearch = searchLower;
        s_filteredAssets.clear();

        if (shouldShowResults)
        {
            for (const FAssetMeta* meta : s_sortedAssets)
            {
                if (!meta)
                    continue;

                if (!Is_EffectAssetInAllowedRelativeRoot(meta, allowedRelativePrefix))
                    continue;

                const string fileName = fs::path(meta->fullPath).filename().string();
                const string relativePath = Utils::ToString(meta->relativePath);
                const string searchTarget = Utils::ToLowerCopy(fileName + " " + relativePath);

                if (!searchLower.empty() && searchTarget.find(searchLower) == string::npos)
                    continue;

                s_filteredAssets.push_back(meta);
            }
        }
    }

    ImGui::BeginChild("EffectAssetPickerList", ImVec2(620.f, 320.f), true);

    if (s_sortedAssets.empty())
    {
        ImGui::TextDisabled("(등록된 에셋 없음)");
    }
    else if (!shouldShowResults)
    {
        ImGui::TextDisabled("검색어를 입력하면 결과가 표시됩니다.");
    }
    else if (ImGui::BeginTable("EffectAssetPickerTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("Thumb", ImGuiTableColumnFlags_WidthFixed, _assetPickerThumbnailSize + 8.f);
        ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthStretch);
        if (s_filteredAssets.empty())
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(1.f, _assetPickerThumbnailSize));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("(검색 결과 없음)");
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int32>(s_filteredAssets.size()), _assetPickerThumbnailSize + ImGui::GetStyle().CellPadding.y * 2.f);

            while (clipper.Step())
            {
                for (int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    const FAssetMeta* meta = s_filteredAssets[i];
                    if (!meta)
                        continue;

                    const string fileName = fs::path(meta->fullPath).filename().string();
                    const string relativePath = Utils::ToString(meta->relativePath);

                    Load_AssetPickerThumbnail(meta->guid);

                    ImGui::PushID(meta->guid.c_str());
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    auto thumbIter = _assetPickerThumbnailCache.find(meta->guid);
                    if (thumbIter != _assetPickerThumbnailCache.end())
                    {
                        ImGui::Image((ImTextureID)thumbIter->second.Get(), ImVec2(_assetPickerThumbnailSize, _assetPickerThumbnailSize));
                    }
                    else
                    {
                        ImGui::Button("##NoThumb", ImVec2(_assetPickerThumbnailSize, _assetPickerThumbnailSize));
                    }

                    ImGui::TableSetColumnIndex(1);

                    const bool isSelected = (meta->guid == targetGuid);
                    const string label = Build_EffectAssetPickerLabel(*meta);

                    if (isSelected)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.85f, 0.35f, 1.f));

                    if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0.f, _assetPickerThumbnailSize)))
                    {
                        if (Commit_AssetGuidChange(targetGuid, meta->guid, expectedAssetType, allowedRelativePrefix, outResourceChanged))
                            ImGui::CloseCurrentPopup();
                    }

                    if (isSelected)
                        ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", fileName.c_str());
                        ImGui::TextDisabled("%s", relativePath.c_str());
                        ImGui::Text("GUID: %s", meta->guid.c_str());
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();
                }
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Selectable("(None) - 해제", false))
    {
        if (Commit_AssetGuidChange(targetGuid, "", expectedAssetType, allowedRelativePrefix, outResourceChanged))
            ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void Effect_View::Move_SelectedLayer(int32 direction)
{
    if (_selectedLayerIdx < 0 || _selectedLayerIdx >= static_cast<int32>(_currentAsset.layers.size()))
        return;

    const int32 targetIdx = _selectedLayerIdx + direction;
    if (targetIdx < 0 || targetIdx >= static_cast<int32>(_currentAsset.layers.size()))
        return;

    std::swap(_currentAsset.layers[_selectedLayerIdx], _currentAsset.layers[targetIdx]);
    _selectedLayerIdx = targetIdx;

    MarkDirty();
    Restart_PreviewEffect();
}

void Effect_View::Draw_ResolvedAssetInfo(const char* label, const string& guid) const
{
    if (guid.empty())
    {
        ImGui::TextDisabled("%s: <empty>", label);
        return;
    }

    wstring resolvedPath = GAME->Resolve_AssetPath(guid);
    if (resolvedPath.empty())
    {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s: Invalid GUID", label);
        return;
    }

    ImGui::TextDisabled("%s: %s", label, fs::path(resolvedPath).filename().string().c_str());
}

void Effect_View::Apply_SelectedLayerPreview(bool transformChanged, bool materialChanged, bool resourceChanged)
{
    if (!_previewEffectCom)
        return;

    if (_selectedLayerIdx < 0 || _selectedLayerIdx >= static_cast<int32>(_currentAsset.layers.size()))
        return;

    const auto& layerDesc = _currentAsset.layers[_selectedLayerIdx];

    if (resourceChanged || materialChanged)
    {
        _previewEffectCom->Apply_LayerDesc(_selectedLayerIdx, layerDesc, resourceChanged);
        return;
    }

    if (transformChanged)
    {
        _previewEffectCom->Apply_LayerTransform(_selectedLayerIdx, layerDesc.base);
    }
}

void Effect_View::Restart_PreviewEffect()
{
    if (!_previewEffectCom) return;

    _previewEffectCom->Set_ForceVisiblePreview(_forceVisiblePreview);
    _previewEffectCom->Play_EffectAsset(_currentAsset);

    _isPlaying = true;
}

Shared<Effect_View> Effect_View::Create()
{
    return make_shared<Effect_View>();
}
