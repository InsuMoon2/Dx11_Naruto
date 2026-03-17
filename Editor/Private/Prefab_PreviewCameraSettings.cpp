#include "pch.h"
#include "Prefab_PreviewCameraSettings.h"
#include <fstream>

static const char* PREFAB_PREVIEW_CAMERA_SETTINGS_PATH =
"../../Client/Bin/Resources/Data/json/EditorSettings/PrefabPreviewCamera.json";

Editor_Camera_Free::FEditorCameraDesc Prefab_PreviewCameraSettings::Build_Desc() const
{
    Editor_Camera_Free::FEditorCameraDesc desc;
    desc.speedPerSec = speedPerSec;
    desc.rotationPerSec = rotationPerSec;
    desc.eye = eye;
    desc.at = at;
    desc.fovY = XMConvertToRadians(fovYDeg);
    desc.nearZ = nearZ;
    desc.farZ = farZ;
    desc.mouseSensor = mouseSensor;
    return desc;
}

void Prefab_PreviewCameraSettings::Capture_FromCamera(Shared<Editor_Camera_Free> previewCamera)
{
    if (!previewCamera)
        return;

    auto transform = previewCamera->Get_Component<Transform>();
    if (!transform)
        return;

    // 현재 카메라 위치를 다음 시작 eye 값으로 저장한다.
    eye = transform->Get_WorldPosition();

    // 현재 카메라가 바라보는 방향을 기준으로 look-at을 다시 만든다.
    // eye/at 구조는 유지하되, 사용자는 직접 숫자를 계산하지 않아도 된다.
    at = eye + transform->Get_WorldForward();

    // 현재 카메라 설정값도 같이 저장한다.
    speedPerSec = previewCamera->Get_CameraSpeed();
    mouseSensor = previewCamera->Get_MouseSensor();
    fovYDeg = XMConvertToDegrees(previewCamera->Get_FovY());
    nearZ = previewCamera->Get_NearZ();
    farZ = previewCamera->Get_FarZ();
}

void Prefab_PreviewCameraSettings::Save_Settings() const
{
    json root;
    root["speedPerSec"] = speedPerSec;
    root["rotationPerSec"] = rotationPerSec;
    root["eye"] = { eye.x, eye.y, eye.z };
    root["at"] = { at.x, at.y, at.z };
    root["fovYDeg"] = fovYDeg;
    root["nearZ"] = nearZ;
    root["farZ"] = farZ;
    root["mouseSensor"] = mouseSensor;

    fs::path path = PREFAB_PREVIEW_CAMERA_SETTINGS_PATH;
    if (!fs::exists(path.parent_path()))
        fs::create_directories(path.parent_path());

    ofstream file(path);
    if (!file.is_open())
        return;

    file << root.dump(4);
}

void Prefab_PreviewCameraSettings::Load_Settings()
{
    ifstream file(PREFAB_PREVIEW_CAMERA_SETTINGS_PATH);
    if (!file.is_open())
        return;

    json root;
    file >> root;

    speedPerSec = root.value("speedPerSec", speedPerSec);
    rotationPerSec = root.value("rotationPerSec", rotationPerSec);
    fovYDeg = root.value("fovYDeg", fovYDeg);
    nearZ = root.value("nearZ", nearZ);
    farZ = root.value("farZ", farZ);
    mouseSensor = root.value("mouseSensor", mouseSensor);

    if (root.contains("eye") && root["eye"].is_array() && root["eye"].size() == 3)
    {
        eye = Vec3(root["eye"][0].get<float>(), root["eye"][1].get<float>(), root["eye"][2].get<float>());
    }

    if (root.contains("at") && root["at"].is_array() && root["at"].size() == 3)
    {
        at = Vec3(root["at"][0].get<float>(), root["at"][1].get<float>(), root["at"][2].get<float>());
    }
}

Shared<Prefab_PreviewCameraSettings> Prefab_PreviewCameraSettings::Create()
{
    return make_shared<Prefab_PreviewCameraSettings>();
}
