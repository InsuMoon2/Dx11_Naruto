#include "pch.h"
#include "Cinematic_View.h"
#include "CameraTrack_Serializer.h"
#include "CameraTrack_Player.h"
#include "Camera_Cinematic.h"
#include "RenderTarget.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Hierarchy.h"
#include "Transform.h"
#include "Notification_Manager.h"
#include "Scene_View.h"
#include "magic_enum/magic_enum.hpp"

static Vec3 Convert_WorldPosition_ToAnchorLocal(const Vec3& worldPos, Shared<Transform> anchorTransform)
{
    if (!anchorTransform)
        return worldPos;

    Matrix inverseWorld = anchorTransform->Get_WorldMatrix().Invert();
    return Vec3::Transform(worldPos, inverseWorld);
}

static Quat Convert_WorldRotation_ToAnchorLocal(const Quat& worldRot, Shared<Transform> anchorTransform)
{
    if (!anchorTransform)
        return worldRot;

    Quat anchorWorldRot = anchorTransform->Get_WorldRotation();
    Quat anchorInverse = anchorWorldRot;
    anchorInverse.Inverse(anchorInverse);

    return worldRot * anchorInverse;
}

// target_tag 문자열로 현재 레벨의 실제 오브젝트를 찾을 때 사용한다.
// 현재 시네마틱 데이터는 별도 Tag 시스템이 아니라 이름 문자열 기반으로 사용되고 있다.
static Shared<GameObject> Find_CinematicTargetObject(const string& targetTag)
{
    if (targetTag.empty())
        return nullptr;

    const auto objects = GAME->Get_GameObjects(GAME->Current_Level());
    for (const auto& obj : objects)
    {
        if (!obj)
            continue;

        if (Utils::ToString(obj->Get_Name()) == targetTag)
            return obj;
    }

    return nullptr;
}

// 현재 프레임 카메라 키 설정을 Preview 카메라에 반영할 때 호출한다.
// OwnerRelative + Target / LookAt 조합에서도 실제 gameplay 카메라와 비슷한 동작을 만들기 위해 사용한다.
static void Apply_TrackPlayerState_ToPreviewCamera(
    const Shared<CameraTrack_Player>& player,
    const Shared<Camera_Cinematic>& previewCamera,
    const Shared<GameObject>& anchorObject)
{
    if (!player || !previewCamera)
        return;

    const FCameraKey& currentKey = player->Get_CurrentKey();

    previewCamera->Set_Mode(currentKey.cameraMode);
    previewCamera->Set_Distance(currentKey.distance);
    previewCamera->Set_TargetOffset(currentKey.targetOffset);
    previewCamera->Set_PitchYaw(currentKey.pitch, currentKey.yaw);

    Shared<Transform> targetTransform = nullptr;

    if (currentKey.cameraMode == ECineCameraMode::Target ||
        currentKey.cameraMode == ECineCameraMode::LookAt)
    {
        if (!currentKey.targetTag.empty())
        {
            auto targetObject = Find_CinematicTargetObject(currentKey.targetTag);
            if (targetObject)
                targetTransform = targetObject->Get_Transform();
        }

        // target_tag가 비어 있으면 anchor를 기본 타겟으로 사용한다.
        // 이렇게 해야 OwnerRelative 카메라가 플레이어에 장착됐을 때 실제 플레이 화면처럼 플레이어 근처를 따라간다.
        if (!targetTransform && anchorObject)
            targetTransform = anchorObject->Get_Transform();
    }

    previewCamera->Set_TargetTransform(targetTransform);
}

Cinematic_View::Cinematic_View()
    : EditorWindow(TEXT("Cinematic View"))
    , _sequencerAdapter(&_sequencerState, &_sequencerContext)
{
}

void Cinematic_View::Initialize()
{
    EditorWindow::Initialize();

    _sequencerContext.view = this;
    _sequencerContext.track = nullptr;
    _sequencerContext.selectedKeyIndex = &_selectedKeyIndex;

    _player = CameraTrack_Player::Create();
    CHECK_NULL(_player);

    _isActive = false;

}

void Cinematic_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    Sync_PreviewAnchor();

    if (_previewCamera)
    {
        _previewCamera->Set_InputEnabled(!_isPlaying && _isPreviewHovered);
    }

    if (_isPlaying && _player)
    {
        _player->Tick(timeDelta);
        _sequencerState.currentFrame = _player->Get_CurrentFrame();
        if (_previewCamera)
        {
            Apply_TrackPlayerState_ToPreviewCamera(
                _player,
                _previewCamera,
                _previewAnchorObject.lock());

            _previewCamera->Apply_CinematicState(
                _player->Get_Position(),
                _player->Get_Rotation(),
                _player->Get_FovY());
        }
    }

    if (_previewCamera)
    {
        _previewCamera->Priority_Update(timeDelta);
        _previewCamera->Update(timeDelta);
        _previewCamera->Late_Update(timeDelta);
    }

    if (_player && _player->IsFinished())
        _isPlaying = false;
}

void Cinematic_View::OnGui()
{
    string title = Utils::ToString(Get_Name());

    if (!ImGui::Begin(title.c_str(), &_isActive, ImGuiWindowFlags_NoDocking))
    {
        _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGui::End();
        return;
    }

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    Handle_PlaybackShortcut();
    Draw_MenuBar();

    ImGui::Separator();

    Draw_TopLayout();

    Draw_PlayBar();

    //ImGui::Separator();

    Draw_Sequencer();
    Draw_FilePopup();

    ImGui::End();
}

void Cinematic_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    if (!_previewCamera || !_isActive)
        return;

    // Scene_View 에디터 카메라와 충돌 방지
    if (!_isPlaying && !_isFocused)
        return;

    // 시네마틱 카메라의 현재 위치/회전으로 View 계산
    _previewView = _previewCamera->Get_ViewMatrix();

    float width = max(1.f, GAME->Get_ViewportWidth());
    float height = max(1.f, GAME->Get_ViewportHeight());
    float aspect = width / height;

    _previewProj = XMMatrixPerspectiveFovLH(
        _previewCamera->Get_FovY(),
        aspect,
        _previewCamera->Get_NearZ(),
        _previewCamera->Get_FarZ());

    GAME->Set_Transform(ETransformState::View, _previewView);
    GAME->Set_Transform(ETransformState::Proj, _previewProj);
}

bool Cinematic_View::CanSave() const
{
    return !_asset.name.empty();
}

void Cinematic_View::Save()
{
    if (!_currentFilePath.empty())
    {
        Save_Sequence(_currentFilePath);
    }
}

void Cinematic_View::Create_NewSequence()
{
    _asset = {};
    _asset.name = "NewSequence";
    _asset.track.fps = 30;
    _asset.track.totalFrame = 300;
    _selectedKeyIndex = -1;
    _currentFilePath.clear();
    _isPlaying = false;

    Ensure_PreviewCamera();
}

void Cinematic_View::Load_Sequence(const string& path)
{
    FCameraSequenceAsset loadedAsset;
    wstring wpath = Utils::ToWString(path);

    if (CameraTrack_Serializer::Load_FromFile(wpath, loadedAsset))
    {
        _asset = loadedAsset;
        _currentFilePath = path;
        _selectedKeyIndex = -1;
        _isPlaying = false;
        Ensure_PreviewCamera();

        NOTIFY("시퀀스 로드 완료");
    }
}

void Cinematic_View::Save_Sequence(const string& path)
{
    wstring wpath = Utils::ToWString(path);

    if (CameraTrack_Serializer::Save_ToFile(wpath, _asset))
    {
        _currentFilePath = path;
        ClearDirty();
        NOTIFY("시퀀스 저장 완료");
    }
}

void Cinematic_View::Capture_KeyAtCurrentFrame()
{
    if (!_previewCamera)
        return;

    FCameraKey newKey;
    newKey.frame = _sequencerState.currentFrame;
    newKey.fovY = _previewCamera->Get_FovY();
    newKey.cameraMode = _previewCamera->Get_Mode();
    newKey.easeType = ECameraEaseType::Linear;
    newKey.anchorSpace = _captureAnchorSpace;

    auto anchorTransform = Get_SelectedAnchorTransform();

    if (newKey.anchorSpace == ECinemaAnchorSpace::OwnerRelative)
    {
        if (!anchorTransform)
        {
            NOTIFY("OwnerRelative 키는 선택 오브젝트가 필요합니다.");
            return;
        }

        newKey.position = Convert_WorldPosition_ToAnchorLocal(
            _previewCamera->Get_Transform()->Get_WorldPosition(),
            anchorTransform);

        newKey.rotation = Convert_WorldRotation_ToAnchorLocal(
            _previewCamera->Get_Transform()->Get_WorldRotation(),
            anchorTransform);
    }
    else
    {
        newKey.position = _previewCamera->Get_Transform()->Get_WorldPosition();
        newKey.rotation = _previewCamera->Get_Transform()->Get_WorldRotation();
    }

    _asset.track.keys.push_back(newKey);
    Sort_Keys();

    for (int32 i = 0; i < static_cast<int32>(_asset.track.keys.size()); ++i)
    {
        if (_asset.track.keys[i].frame == newKey.frame)
        {
            _selectedKeyIndex = i;
            break;
        }
    }

    MarkDirty();
    NOTIFY("키프레임 추가: F" + to_string(newKey.frame));
}

void Cinematic_View::Delete_SelectedKey()
{
    if (_selectedKeyIndex < 0 ||
        _selectedKeyIndex >= static_cast<int32>(_asset.track.keys.size()))
        return;

    _asset.track.keys.erase(_asset.track.keys.begin() + _selectedKeyIndex);

    _selectedKeyIndex = -1;

    MarkDirty();
}

void Cinematic_View::Draw_MenuBar()
{
    if (ImGui::Button("New"))   Create_NewSequence();
    ImGui::SameLine();

    if (ImGui::Button("Open"))  _isOpenFilePopup = true;
    ImGui::SameLine();

    if (ImGui::Button("Save"))  _isSaveFilePopup = true;
}

void Cinematic_View::Draw_PlayBar()
{
    // 재생 컨트롤
    if (ImGui::Button(_isPlaying ? "Pause" : "Play"))
    {
        if (_isPlaying)
        {
            _isPlaying = false;
            if (_previewCamera) _previewCamera->Set_InputEnabled(true);
        }
        else
        {
            Sync_PreviewAnchor();
            _player->Bind(&_asset.track);
            _player->Seek(_sequencerState.currentFrame);
            _player->Play();
            _isPlaying = true;
            if (_previewCamera) _previewCamera->Set_InputEnabled(false);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        _isPlaying = false;
        Sync_PreviewAnchor();
        _player->Stop();
        _sequencerState.currentFrame = 0;
        Apply_CurrentFrame();
        if (_previewCamera) _previewCamera->Set_InputEnabled(true);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // K = 키프레임 캡처 버튼
    if (ImGui::Button("Add Key [K]"))
        Capture_KeyAtCurrentFrame();

    // 현재 정보 표시
    ImGui::SameLine(0.f, 20.f);

    if (ImGui::BeginCombo("New Key Space",
        string(magic_enum::enum_name(_captureAnchorSpace)).c_str()))
    {
        for (auto space : magic_enum::enum_values<ECinemaAnchorSpace>())
        {
            if (space == ECinemaAnchorSpace::END)
                continue;

            const bool selected = (_captureAnchorSpace == space);
            if (ImGui::Selectable(string(magic_enum::enum_name(space)).c_str(), selected))
                _captureAnchorSpace = space;
        }
        ImGui::EndCombo();
    }

    if (_captureAnchorSpace == ECinemaAnchorSpace::OwnerRelative && !Can_CaptureOwnerRelativeKey())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "선택 오브젝트 필요");
    }

    ImGui::SameLine(0.f, 20.f);
    ImGui::SetNextItemWidth(60.f);
    if (ImGui::InputInt("FPS", &_asset.track.fps, 0, 0))
    {
        _asset.track.fps = std::clamp(_asset.track.fps, 1, 120);
        MarkDirty();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::InputInt("Total Frames", &_asset.track.totalFrame, 0, 0))
    {
        _asset.track.totalFrame = std::max(1, _asset.track.totalFrame);
        MarkDirty();
    }
}

void Cinematic_View::Draw_TopLayout()
{
    static float s_topHeight = 400.f;

    if (ImGui::BeginTable("CineTopLayout", 3,
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
        ImVec2(0.f, s_topHeight)))
    {
        ImGui::TableSetupColumn("KeyList", ImGuiTableColumnFlags_WidthFixed, 200.f);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, 320.f);
        ImGui::TableNextColumn();

        Draw_KeyList(s_topHeight);
        ImGui::TableNextColumn();

        Draw_PreviewPanel(s_topHeight);
        ImGui::TableNextColumn();

        Draw_KeyInspector(s_topHeight);
        ImGui::EndTable();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));       // 평소 색상
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));   // 마우스 올렸을 때
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));    // 클릭 중

    ImGui::Button("##CineSplitter", ImVec2(ImGui::GetContentRegionAvail().x, 6.0f));

    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

    if (ImGui::IsItemActive())
    {
        s_topHeight += ImGui::GetIO().MouseDelta.y;
        s_topHeight = max(150.f, s_topHeight); 
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}


void Cinematic_View::Draw_Sequencer()
{
    _sequencerContext.track = &_asset.track;
    ImVec2 size = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("CineSequencer", size, false);

    ImSequencer::Sequencer(
        &_sequencerAdapter,
        &_sequencerState.currentFrame,
        &_sequencerState.expanded,
        &_sequencerState.selectedEntry,
        &_sequencerState.firstFrame,
        ImSequencer::SEQUENCER_CHANGE_FRAME);

    if (!_isPlaying)
        Apply_CurrentFrame();

    ImGui::EndChild();
}

void Cinematic_View::Draw_KeyList(float height)
{
    ImGui::Text("Keys (%d)", static_cast<int>(_asset.track.keys.size()));
    ImGui::Separator();

    float childHeight = height - ImGui::GetCursorPosY() - 5.f;
    if (ImGui::BeginChild("CineKeyList", ImVec2(0.f, max(10.f, childHeight)), true))
    {
        for (int32 i = 0; i < static_cast<int32>(_asset.track.keys.size()); ++i)
        {
            const auto& key = _asset.track.keys[i];
            string label = string(magic_enum::enum_name(key.cameraMode))
                + " / "
                + string(magic_enum::enum_name(key.anchorSpace))
                + " [F" + to_string(key.frame) + "]";
            if (ImGui::Selectable(label.c_str(), _selectedKeyIndex == i))
                _selectedKeyIndex = i;
        }
    }
    ImGui::EndChild();
}

void Cinematic_View::Draw_PreviewPanel(float height)
{
    ImGui::Text("Preview");

    if (auto anchor = _previewAnchorObject.lock())
    {
        ImGui::TextDisabled("Anchor: %s", Utils::ToString(anchor->Get_Name()).c_str());
    }
    else
    {
        ImGui::TextDisabled("Anchor: None (World Fallback)");
    }

    float childHeight = height - ImGui::GetCursorPosY() - 5.f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    avail.y = max(10.f, childHeight);

    ImGui::BeginChild("CinePreview", avail, true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    _isPreviewHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    // Scene_View의 RT를 빌려서 표시
    auto sceneView = dynamic_pointer_cast<Scene_View>(
        EDITOR->Get_Window(TEXT("Scene"))); 

    if (sceneView && sceneView->Get_RenderTarget())
    {
        ImVec2 drawSize = avail;
        float texAspect = max(1.f, GAME->Get_ViewportWidth()) /
            max(1.f, GAME->Get_ViewportHeight());

        if (avail.x / texAspect <= avail.y)
        {
            drawSize.x = avail.x; drawSize.y = avail.x / texAspect;
        }
        else
        {
            drawSize.y = avail.y; drawSize.x = avail.y * texAspect;
        }

        ImVec2 cursor = ImGui::GetCursorPos();
        cursor.x += (avail.x - drawSize.x) * 0.5f;
        cursor.y += (avail.y - drawSize.y) * 0.5f;
        ImGui::SetCursorPos(cursor);

        ImGui::Image(
            (ImTextureID)sceneView->Get_RenderTarget()->Get_SRV(),
            drawSize);
    }
    else
    {
        ImGui::TextDisabled("Scene View RT 없음");
    }

    ImGui::EndChild();
}

void Cinematic_View::Draw_KeyInspector(float height)
{
    ImGui::Text("Key Inspector");
    ImGui::Separator();

    float childHeight = height - ImGui::GetCursorPosY() - 5.f;
    if (ImGui::BeginChild("CineKeyInspector", ImVec2(0.f, max(10.f, childHeight)), true))
    {
        if (_selectedKeyIndex < 0 ||
            _selectedKeyIndex >= static_cast<int32>(_asset.track.keys.size()))
        {
            ImGui::TextDisabled("키프레임을 선택하세요");
            ImGui::EndChild();
            return;
        }
        auto& key = _asset.track.keys[_selectedKeyIndex];
        bool isDirty = false;
        bool needResort = false;

        if (ImGui::BeginCombo("Anchor Space", string(magic_enum::enum_name(key.anchorSpace)).c_str()))
        {
            for (auto space : magic_enum::enum_values<ECinemaAnchorSpace>())
            {
                if (space == ECinemaAnchorSpace::END)
                    continue;

                const bool selected = (key.anchorSpace == space);
                if (ImGui::Selectable(string(magic_enum::enum_name(space)).c_str(), selected))
                {
                    key.anchorSpace = space;
                    isDirty = true;
                }
            }
            ImGui::EndCombo();
        }

        if (key.anchorSpace == ECinemaAnchorSpace::OwnerRelative && !_previewAnchorObject.lock())
        {
            ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f),
                "Anchor 없음: 현재 프리뷰는 World Fallback으로 표시됩니다.");
        }

        // 프레임
        const int32 prevFrame = key.frame;
        if (ImGui::InputInt("Frame", &key.frame))
        {
            key.frame = std::clamp(key.frame, 0, _asset.track.totalFrame);
            if (prevFrame != key.frame)
            {
                needResort = true;
                isDirty = true;
            }
        }

        // 카메라 모드 콤보
        if (ImGui::BeginCombo("Mode", string(magic_enum::enum_name(key.cameraMode)).c_str()))
        {
            for (auto mode : magic_enum::enum_values<ECineCameraMode>())
            {
                if (mode == ECineCameraMode::END)
                    continue;

                bool selected = (key.cameraMode == mode);

                if (ImGui::Selectable(string(magic_enum::enum_name(mode)).c_str(), selected))
                {
                    key.cameraMode = mode;
                    isDirty = true;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // 공통 속성
        const bool isOwnerRelative = (key.anchorSpace == ECinemaAnchorSpace::OwnerRelative);
        const char* positionLabel = isOwnerRelative ? "Local Offset" : "Position";
        const char* rotationLabel = isOwnerRelative ? "Local Rotation (deg)" : "Rotation (deg)";

        if (ImGui::DragFloat3(positionLabel, &key.position.x, 0.1f))
            isDirty = true;

        Vec3 euler = key.rotation.ToEuler();
        euler.x = XMConvertToDegrees(euler.x);
        euler.y = XMConvertToDegrees(euler.y);
        euler.z = XMConvertToDegrees(euler.z);

        if (ImGui::DragFloat3(rotationLabel, &euler.x, 0.5f))
        {
            key.rotation = Quat::CreateFromYawPitchRoll(
                XMConvertToRadians(euler.y),
                XMConvertToRadians(euler.x),
                XMConvertToRadians(euler.z));
            isDirty = true;
        }

        float fovDeg = XMConvertToDegrees(key.fovY);
        if (ImGui::SliderFloat("FoV", &fovDeg, 10.f, 120.f))
        {
            key.fovY = XMConvertToRadians(fovDeg);
            isDirty = true;
        }

        // 이징
        if (ImGui::BeginCombo("Ease", string(magic_enum::enum_name(key.easeType)).c_str()))
        {
            for (auto ease : magic_enum::enum_values<ECameraEaseType>())
            {
                if (ease == ECameraEaseType::END) continue;
                bool selected = (key.easeType == ease);
                if (ImGui::Selectable(string(magic_enum::enum_name(ease)).c_str(), selected))
                {
                    key.easeType = ease;
                    isDirty = true;
                }
            }
            ImGui::EndCombo();
        }

        // ─── 모드별 추가 속성 ───
        if (key.cameraMode == ECineCameraMode::Target ||
            key.cameraMode == ECineCameraMode::LookAt)
        {
            ImGui::Separator();
            ImGui::Text("Target Settings");
            char tagBuf[128] = {};

            strcpy_s(tagBuf, key.targetTag.c_str());

            if (ImGui::InputText("Target Tag", tagBuf, sizeof(tagBuf)))
            {
                key.targetTag = tagBuf;
                isDirty = true;
            }

            if (ImGui::DragFloat3("Target Offset", &key.targetOffset.x, 0.1f))
                isDirty = true;
        }

        if (key.cameraMode == ECineCameraMode::Target)
        {
            if (ImGui::DragFloat("Distance", &key.distance, 0.1f, 1.f, 50.f))
                isDirty = true;
            if (ImGui::DragFloat("Pitch", &key.pitch, 0.5f, -80.f, 80.f))
                isDirty = true;
            if (ImGui::DragFloat("Yaw", &key.yaw, 0.5f))
                isDirty = true;
        }

        ImGui::Separator();

        if (ImGui::Button("Delete Key"))
            Delete_SelectedKey();

        if (needResort)
        {
            const int32 targetFrame = key.frame;
            const ECineCameraMode targetMode = key.cameraMode;
            const ECinemaAnchorSpace targetSpace = key.anchorSpace;

            Sort_Keys();

            for (int32 i = 0; i < static_cast<int32>(_asset.track.keys.size()); ++i)
            {
                const auto& sortedKey = _asset.track.keys[i];
                if (sortedKey.frame == targetFrame &&
                    sortedKey.cameraMode == targetMode &&
                    sortedKey.anchorSpace == targetSpace)
                {
                    _selectedKeyIndex = i;
                    break;
                }
            }
        }

        if (isDirty)
            MarkDirty();
    }
    ImGui::EndChild();
}

void Cinematic_View::Draw_FilePopup()
{
    // Open 팝업
    if (_isOpenFilePopup)
    {
        ImGui::OpenPopup("Open Cinematic");
        _isOpenFilePopup = false;
    }

    if (ImGui::BeginPopupModal("Open Cinematic", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        fs::path folder = Get_FolderPath();
        if (fs::exists(folder))
        {
            for (auto& entry : fs::directory_iterator(folder))
            {
                if (entry.path().extension() == ".json")
                {
                    string name = entry.path().stem().string();
                    if (ImGui::Selectable(name.c_str()))
                    {
                        Load_Sequence(entry.path().string());
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Save 팝업
    if (_isSaveFilePopup)
    {
        ImGui::OpenPopup("Save Cinematic");
        _isSaveFilePopup = false;
    }

    if (ImGui::BeginPopupModal("Save Cinematic", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("File Name", _saveFileNameBuf, sizeof(_saveFileNameBuf));
        if (ImGui::Button("Save") && strlen(_saveFileNameBuf) > 0)
        {
            fs::path savePath = Get_FolderPath() / (string(_saveFileNameBuf) + ".json");
            _asset.name = _saveFileNameBuf;
            Save_Sequence(savePath.string());
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void Cinematic_View::Handle_PlaybackShortcut()
{
    if (!_isFocused) return;

    const bool isSavePopupOpen = ImGui::IsPopupOpen("Save Cinematic");

    if (ImGui::IsKeyPressed(ImGuiKey_Space))
    {
        if (_isPlaying)
        {
            _isPlaying = false; if (_previewCamera) _previewCamera->Set_InputEnabled(true);
        }
        else
        {
            Sync_PreviewAnchor();
            _player->Bind(&_asset.track);
            _player->Seek(_sequencerState.currentFrame);
            _player->Play();
            _isPlaying = true;
            if (_previewCamera) _previewCamera->Set_InputEnabled(false);
        }
    }

    if (!isSavePopupOpen && ImGui::IsKeyPressed(ImGuiKey_K) && !_isPlaying)
        Capture_KeyAtCurrentFrame();

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && !_isPlaying)
    {
        _sequencerState.currentFrame = max(0, _sequencerState.currentFrame - 1);
        Apply_CurrentFrame();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && !_isPlaying)
    {
        _sequencerState.currentFrame = min(_asset.track.totalFrame,
            _sequencerState.currentFrame + 1);
        Apply_CurrentFrame();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Home))
    {
        _sequencerState.currentFrame = 0; Apply_CurrentFrame();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_End))
    {
        _sequencerState.currentFrame = _asset.track.totalFrame; Apply_CurrentFrame();
    }

    // 모드 전환: 1/2/3/4
    if (_previewCamera)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_1))
            _previewCamera->Set_Mode(ECineCameraMode::Free);

        if (ImGui::IsKeyPressed(ImGuiKey_2))
            _previewCamera->Set_Mode(ECineCameraMode::Target);

        if (ImGui::IsKeyPressed(ImGuiKey_3))
            _previewCamera->Set_Mode(ECineCameraMode::LookAt);

        if (ImGui::IsKeyPressed(ImGuiKey_4))
            _previewCamera->Set_Mode(ECineCameraMode::Rail);
    }
}

void Cinematic_View::Apply_CurrentFrame()
{
    if (!_previewCamera || _asset.track.keys.empty()) return;

    Sync_PreviewAnchor();
    _player->Bind(&_asset.track);
    _player->Seek(_sequencerState.currentFrame);

    Apply_TrackPlayerState_ToPreviewCamera(
        _player,
        _previewCamera,
        _previewAnchorObject.lock());

    _previewCamera->Apply_CinematicState(
        _player->Get_Position(),
        _player->Get_Rotation(),
        _player->Get_FovY());
}

void Cinematic_View::Ensure_PreviewCamera()
{
    if (_previewCamera) return;

    Camera_Cinematic::FCinematicDesc desc;

    // 에디터 카메라 위치를 기본값으로 사용
    auto activeCamera = GAME->Get_ActiveCamera();
    if (activeCamera)
    {
        auto camTransform = activeCamera->Get_Component<Transform>();
        if (camTransform)
        {
            desc.eye = camTransform->Get_WorldPosition();
            desc.at = desc.eye + camTransform->Get_WorldForward() * 10.f;
        }
    }

    else
    {
        desc.eye = Vec3(0.f, 5.f, -10.f);
        desc.at = Vec3(0.f, 0.f, 0.f);
    }

    desc.mode = ECineCameraMode::Free;
    desc.mouseSensor = 10.f;

    _previewCamera = Camera_Cinematic::Create(GAME->Get_Device(), GAME->Get_Context());

    if (_previewCamera)
    {
        auto clone = dynamic_pointer_cast<Camera_Cinematic>(
            _previewCamera->Clone(&desc));
        if (clone) _previewCamera = clone;
    }
}

void Cinematic_View::Sort_Keys()
{
    sort(_asset.track.keys.begin(), _asset.track.keys.end(),
        [](const FCameraKey& a, const FCameraKey& b) { return a.frame < b.frame; });
}

Shared<GameObject> Cinematic_View::Get_SelectedAnchorObject() const
{
    auto hierarchy = dynamic_pointer_cast<Hierarchy>(
        EDITOR->Get_Window(TEXT("Hierarchy")));

    if (!hierarchy)
        return nullptr;

    const auto& selected = hierarchy->Get_SelectedObject();
    if (selected.empty())
        return nullptr;

    return selected.front();
}

Shared<Transform> Cinematic_View::Get_SelectedAnchorTransform() const
{
    auto selectedObject = Get_SelectedAnchorObject();
    if (!selectedObject)
        return nullptr;

    return selectedObject->Get_Transform();
}

void Cinematic_View::Sync_PreviewAnchor()
{
    auto selectedObject = Get_SelectedAnchorObject();

    bool anchorChanged = (selectedObject != _previewAnchorObject.lock());

    _previewAnchorObject = selectedObject;

    if (_player)
    {
        if (selectedObject)
            _player->Set_AnchorTransform(selectedObject->Get_Transform());
        else
            _player->Clear_AnchorTransform();
    }

    // 앵커가 새로 세팅됐고, 키 프레임이 없는 초기 상태면 위치 근처로 이동
    if (anchorChanged && selectedObject && _previewCamera && _asset.track.keys.empty())
    {
        auto anchorTransform = selectedObject->Get_Transform();
        if (anchorTransform)
        {
            Vec3 anchorPos = anchorTransform->Get_WorldPosition();
            Vec3 camPos = anchorPos + Vec3(0.f, 3.f, -8.f);

            Vec3 lookTarget = anchorPos + Vec3(0.f, 1.5f, 0.f); 
            _previewCamera->Get_Transform()->Set_LocalPosition(camPos);
            _previewCamera->Get_Transform()->LookAt(lookTarget);
        }
    }
}

bool Cinematic_View::Can_CaptureOwnerRelativeKey() const
{
    return Get_SelectedAnchorTransform() != nullptr;
}

Shared<Cinematic_View> Cinematic_View::Create()
{
    auto instance = make_shared<Cinematic_View>();

    instance->Initialize();

    return instance;
}
