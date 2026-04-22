#pragma once

NS_BEGIN(Engine)

// 시네마틱 카메라
enum class ECineCameraMode : uint8
{
    Free, Target, LookAt, Rail, END
};

// 보간 타입
enum class ECameraEaseType : uint8
{
    Linear, EaseIn, EaseOut, EaseInOut, END
};

// 카메라 키가 어떤 기준 공간에서 세팅되는지
// World -> 맵 시네마틱
// Owner -> 시전자 기준 로컷 오프셋/회전
enum class ECinemaAnchorSpace : uint8
{
    WorldAbsolute,
    OwnerRelative,

    END
};

// 단일 키프레임
struct FCameraKey
{
    int32           frame = 0;
    ECineCameraMode cameraMode = ECineCameraMode::Free; // 기본 Free

    Vec3            position = Vec3::Zero;
    Quat            rotation = Quat::Identity;
    float           fovY  = XM_PIDIV4; // 기본 45도
    ECameraEaseType easeType = ECameraEaseType::Linear;

    ECinemaAnchorSpace anchorSpace = ECinemaAnchorSpace::WorldAbsolute;

    // Target / LookAt 모드 전용
    string          targetTag;      // 레벨에서 찾을 오브젝트 태그
    float           distance = 10.f;
    Vec3            targetOffset = Vec3(0.f, 2.f, 0.f);
    float           pitch = 0.f;
    float           yaw = 0.f;      // 둘다 degree 값으로 사용
};

// 트랙 = 키프레임 시퀀스
struct FCameraTrack
{
    string              trackName = "Camera";
    int32               fps = 30;
    int32               totalFrame = 300; // 10초 : 30fps
    vector<FCameraKey>  keys;
};

// 시퀀스 에셋
struct FCameraSequenceAsset
{
    string          name;
    FCameraTrack    track;
};

struct FCameraShakeDesc
{
    string tag = ""; // 같은 카메라 셰이크들어오는거 판별용
    float durationSec = 0.15f;
    float frequency = 24.f;
    float blendInSec = 0.01f;
    float blendOutSec = 0.08f;

    Vec3 localPosAmplitude = Vec3::Zero;
    Vec3 localRotAmplitudeDeg = Vec3::Zero;
};

NS_END
