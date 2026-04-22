#include "pch.h"
#include "AN_CameraShake.h"
#include "AnimNotify_Factory.h"
#include "Camera_Types.h"
#include "Utils.h"

REGISTER_ANIM_NOTIFY(AN_CameraShake)
IMPLEMENT_REFLECTION(AN_CameraShake)

bool AN_CameraShake::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_CameraShake";
    info.properties.clear();

    PROPERTY_STRING_JSON("태그", "tag", _tag);
    PROPERTY_FLOAT_JSON("지속 시간", "duration_sec", _durationSec, 0.01f, 10.f);
    PROPERTY_FLOAT_JSON("주파수", "frequency", _frequency, 1.f, 10.f);
    PROPERTY_FLOAT_JSON("블렌드 인", "blend_in_sec", _blendInSec, 0.01f,  10.f);
    PROPERTY_FLOAT_JSON("블렌드 아웃", "blend_out_sec", _blendOutSec, 0.01f,  10.f);
    PROPERTY_VEC3_JSON("위치 진폭", "local_pos_amplitude", _localPosAmplitude, 0.01f);
    PROPERTY_VEC3_JSON("회전 진폭", "local_rot_amplitude_deg", _localRotAmplitudeDeg, 0.1f);

    return true;
}

void AN_CameraShake::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    FCameraShakeDesc request{};
    request.tag = _tag;
    request.durationSec = _durationSec;
    request.frequency = _frequency;
    request.blendInSec = _blendInSec;
    request.blendOutSec = _blendOutSec;
    request.localPosAmplitude = _localPosAmplitude;
    request.localRotAmplitudeDeg = _localRotAmplitudeDeg;

    GAME->Request_CameraShake(request);
}
