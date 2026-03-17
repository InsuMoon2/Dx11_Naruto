#pragma once

#include "Base.h"
#include "Editor_Camera_Free.h"

NS_BEGIN(Editor)

class Prefab_PreviewCameraSettings : public Base
{
    GENERATED_BODY(Prefab_PreviewCameraSettings)

public:
    Prefab_PreviewCameraSettings() = default;
    virtual ~Prefab_PreviewCameraSettings() = default;

public:
    // 프리팹 뷰에서만 쓰는 "시작용 기본값" 세팅용
    float   speedPerSec = 20.f;
    float   rotationPerSec = 90.f;
    Vec3    eye = Vec3(-7.f, 3.f, -10.f);
    Vec3    at = Vec3(6.f, 0.f, 0.f);
    float   fovYDeg = 45.f;
    float   nearZ = 0.1f;
    float   farZ = 1000.f;
    float   mouseSensor = 0.15f;

public:
    Editor_Camera_Free::FEditorCameraDesc Build_Desc() const;

    // 현재 프리뷰 카메라 Transform/파라미터를 이 설정값으로 다시 저장
    void Capture_FromCamera(Shared<Editor_Camera_Free> previewCamera);

    void Save_Settings() const;
    void Load_Settings();

public:
    static Shared<Prefab_PreviewCameraSettings> Create();

};

NS_END
