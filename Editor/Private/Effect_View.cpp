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
#include "Prefab_PreviewCameraSettings.h"

#include "Notification_Manager.h"

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

    if (ImGui::BeginTable("EffectLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
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

    GAME->Draw();

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
    EffectAsset_Serializer::Save_EffectAsset(_savePath, _currentAsset);

    NOTIFY("이펙트 저장");
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

void Effect_View::Draw_Header()
{
    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save", ImVec2(100.f, 0)))
        Save();

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_FILE_ARROW_DOWN " Load", ImVec2(100.f, 0)))
    {
        Engine::EffectAsset_Serializer::Load_EffectAsset(_savePath, _currentAsset);
        Restart_PreviewEffect();
    }
    ImGui::SameLine();

    if (ImGui::Button("Restart Effect")) Restart_PreviewEffect();

    ImGui::SameLine();
    ImGui::Checkbox("Preview Grid", &_showPreviewGrid);
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

    for (int i = 0; i < _currentAsset.layers.size(); ++i)
    {
        ImGui::PushID(i);

        auto& layer = _currentAsset.layers[i];

        string layerTitle = "[" + string(layer.base.kind == Engine::EEffectLayerKind::Mesh ? "Mesh" : "Point") + "] " + layer.base.layerName;
        if (ImGui::Selectable(layerTitle.c_str(), _selectedLayerIdx == i))
            _selectedLayerIdx = i;

        ImGui::PopID();
    }

    ImGui::Spacing();

    if (ImGui::Button("+ Add Mesh Emitter", ImVec2(-1, 30.f)))
    {
        Engine::FEffectLayerDesc newLayer;
        newLayer.base.layerName = "NewMeshEmt";
        newLayer.base.kind = Engine::EEffectLayerKind::Mesh;
        _currentAsset.layers.push_back(newLayer);

        Restart_PreviewEffect();
    }

    ImGui::Spacing();

    if (_selectedLayerIdx >= 0 && _selectedLayerIdx < _currentAsset.layers.size())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button(ICON_FA_TRASH " Delete Selected", ImVec2(-1, 30.f)))
        {
            _currentAsset.layers.erase(_currentAsset.layers.begin() + _selectedLayerIdx);
            _selectedLayerIdx = -1; 

            Restart_PreviewEffect(); 
        }
        ImGui::PopStyleColor(3);
    }
}

void Effect_View::Draw_Inspector()
{
    if (_selectedLayerIdx < 0 || _selectedLayerIdx >= _currentAsset.layers.size())
    {
        ImGui::TextDisabled("No Emitter Selection");
        return;
    }
    auto& layer = _currentAsset.layers[_selectedLayerIdx];
    bool needsRestart = false;

    // --- Header ---
    char nameBuf[256];
    strcpy_s(nameBuf, layer.base.layerName.c_str());
    if (ImGui::InputText("Name", nameBuf, 256)) layer.base.layerName = nameBuf;
    if (ImGui::Checkbox("Enabled", &layer.base.enabled)) needsRestart = true;

    // --- Emitter Settings ---
    if (ImGui::CollapsingHeader(ICON_FA_CLOCK " Transform & Time", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat("Duration", &layer.base.duration, 0.1f, -1.f, 100.f)) needsRestart = true;
        if (ImGui::DragFloat("Delay", &layer.base.startDelay, 0.05f)) needsRestart = true;
        if (ImGui::Checkbox("Loop", &layer.base.loop)) needsRestart = true;
        ImGui::DragFloat3("Local Pos", (float*)&layer.base.localPosition, 0.1f);
        ImGui::DragFloat3("Local Rot", (float*)&layer.base.localRotation, 1.0f);
        ImGui::DragFloat3("Local Scale", (float*)&layer.base.localScale, 0.05f);
    }

    // --- Shader / Mesh Settings ---
    if (layer.base.kind == Engine::EEffectLayerKind::Mesh)
    {
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char buf[256];
            strcpy_s(buf, layer.mesh.modelGuid.c_str());
            ImGui::InputText("Model GUID", buf, 256);

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                layer.mesh.modelGuid = buf; needsRestart = true;
            }

            strcpy_s(buf, layer.mesh.diffuseTextureGuid.c_str());
            ImGui::InputText("Diffuse GUID", buf, 256);

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                layer.mesh.diffuseTextureGuid = buf; needsRestart = true;
            }

            strcpy_s(buf, layer.mesh.maskTextureGuid.c_str());
            ImGui::InputText("Mask GUID", buf, 256);

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                layer.mesh.maskTextureGuid = buf; needsRestart = true;
            }

            int blend = (int)layer.mesh.blendMode;
            if (ImGui::Combo("BlendMode", &blend, "Translucent\0Additive\0Opaque\0"))
            {
                layer.mesh.blendMode = (Engine::EEffectBlendMode)blend;
                needsRestart = true;
            }
        }
        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Material Details", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color Tint", (float*)&layer.mesh.colorTint);
            ImGui::DragFloat2("UV Tiling", (float*)&layer.mesh.uvTiling, 0.05f);
            ImGui::DragFloat2("UV Speed", (float*)&layer.mesh.uvScrollSpeed, 0.05f);
            ImGui::SliderFloat("Opacity", &layer.mesh.opacity, 0.f, 1.f);
            ImGui::Spacing();
            ImGui::DragFloat3("Rotation Axis", (float*)&layer.mesh.rotationAxis, 0.05f);
            ImGui::DragFloat("Rotate Speed", &layer.mesh.rotationSpeed, 0.1f);
            ImGui::Spacing();
            ImGui::DragFloat("Fresnel Power", &layer.mesh.fresnelPower, 0.1f);
            ImGui::DragFloat("Fresnel Mul", &layer.mesh.fresnelMultiplier, 0.1f);
        }
    }
    if (needsRestart)
    {
        Restart_PreviewEffect();
    }
}

void Effect_View::Restart_PreviewEffect()
{
    if (!_previewEffectCom) return;

    _previewEffectCom->Play_EffectAsset(_currentAsset);

    _isPlaying = true;
}

Shared<Effect_View> Effect_View::Create()
{
    return make_shared<Effect_View>();
}
