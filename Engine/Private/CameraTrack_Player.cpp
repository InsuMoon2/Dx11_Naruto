#include "pch.h"
#include "CameraTrack_Player.h"

#include "Transform.h"

void CameraTrack_Player::Bind(const FCameraTrack* track)
{
    _track = track;
    Stop();
}

void CameraTrack_Player::Set_AnchorTransform(Shared<Transform> anchorTransform)
{
    _anchorTransform = anchorTransform;
}

void CameraTrack_Player::Clear_AnchorTransform()
{
    _anchorTransform.reset();
}

void CameraTrack_Player::Play()
{
    if (!_track || _track->keys.empty())
        return;

    _isPlaying = true;
}

void CameraTrack_Player::Pause()
{
    _isPlaying = false;
}

void CameraTrack_Player::Stop()
{
    _isPlaying = false;
    _playbackTimeSec = 0.f;
    _currentFrame = 0;
    _currentKey = {};

    if (_track && !_track->keys.empty())
        Evaluate(0); // current 트랙 초기화
}

bool CameraTrack_Player::IsFinished() const
{
    if (!_track)
        return true;

    return _currentFrame >= _track->totalFrame;
}

void CameraTrack_Player::Tick(float timeDelta)
{
    if (!_isPlaying || !_track || _track->keys.empty())
        return;

    _playbackTimeSec += timeDelta;
    
    const float fps = static_cast<float>(_track->fps);
    _currentFrame = static_cast<int32>(_playbackTimeSec * fps); // 소숫점 자르려고 int

    // 트랙 끝 도달
    if (_currentFrame >= _track->totalFrame)
    {
        _currentFrame = _track->totalFrame;
        _isPlaying = false;

        Evaluate(_currentFrame);

        if (OnFinished)
        {
            OnFinished();
        }

        return;
    }

    Evaluate(_currentFrame);
}

void CameraTrack_Player::Seek(int32 frame)
{
    if (!_track)
        return;

    _currentFrame = ::clamp(frame, 0, _track->totalFrame);
    _playbackTimeSec = static_cast<float>(_currentFrame) /
        static_cast<float>(max(1, _track->fps));

    Evaluate(_currentFrame);
}

Vec3 CameraTrack_Player::Lerp_Position(const FCameraKey& a, const FCameraKey& b, float t) const
{
    const auto& keys = _track->keys;
    const int32 count = static_cast<int32>(keys.size());

    int32 indexA = 0, indexB = 0;
    Find_KeySegment(a.frame, indexA, indexB);

    // p0 = 이전-이전 키 없으면 a 반복
    int32 index0 = max(0, indexA - 1);
    // p3 = 다음-다음 키 없으면 b 반복
    int32 index3 = min(count - 1, indexB + 1);

    return CatmullRom(
        keys[index0].position,
        a.position,
        b.position,
        keys[index3].position,
        t);
}

Quat CameraTrack_Player::Lerp_Rotation(const FCameraKey& a, const FCameraKey& b, float t) const
{
    return Quat::Slerp(a.rotation, b.rotation, t);
}

float CameraTrack_Player::Lerp_FovY(const FCameraKey& a, const FCameraKey& b, float time) const
{
    return a.fovY + (b.fovY - a.fovY) * time;
}

Vec3 CameraTrack_Player::CatmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) const
{
    // 표준 Catmull-Rom 공식이라고 하네
    float t2 = t * t;
    float t3 = t2 * t;

    Vec3 result =
        0.5f * ((2.f * p1) +
            (-p0 + p2) * t +
            (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2 +
            (-p0 + 3.f * p1 - 3.f * p2 + p3) * t3);

    return result;
}

float CameraTrack_Player::Apply_Ease(float t, ECameraEaseType type) const
{
    switch (type)
    {
    case ECameraEaseType::EaseIn:
        return t*t;

    case ECameraEaseType::EaseOut:
        return t * (2.f - t);

    case ECameraEaseType::EaseInOut:
        return (t < 0.5f)
            ? 2.f * t * t
            : -1.f + (4.f - 2.f * t) * t;

    case ECameraEaseType::Linear:

    default: return t;
    }
}

bool CameraTrack_Player::Find_KeySegment(int32 frame, int32& prevIdx, int32& nextIdx) const
{
    const auto& keys = _track->keys;

    if (keys.empty())
        return false;

    // 프레임이 첫 키 이전 -> 0으로 세팅
    if (frame <= keys.front().frame)
    {
        prevIdx = nextIdx = 0;
        return true;
    }

    // 프레임이 마지막 키 이후
    if (frame >= keys.back().frame)
    {
        prevIdx = nextIdx = static_cast<int32>(keys.size()) - 1;
        return true;
    }

    // 구간 탐색
    for (int32 i = 0; i < static_cast<int32>(keys.size()) -1; ++i)
    {
        if (frame >= keys[i].frame && frame <= keys[i + 1].frame)
        {
            prevIdx = i;
            nextIdx = i + 1;

            return true;
        }
    }

    return false;
}

Vec3 CameraTrack_Player::Resolve_KeyWorldPosition(const FCameraKey& key) const
{
    if (key.anchorSpace == ECinemaAnchorSpace::WorldAbsolute)
        return key.position;

    auto anchor = _anchorTransform.lock();
    if (!anchor)
        return key.position;

    return Vec3::Transform(key.position, anchor->Get_WorldMatrix());
}

Quat CameraTrack_Player::Resolve_KeyWorldRotation(const FCameraKey& key) const
{
    if (key.anchorSpace == ECinemaAnchorSpace::WorldAbsolute)
        return key.rotation;

    auto anchor = _anchorTransform.lock();
    if (!anchor)
        return key.rotation;

    return key.rotation * anchor->Get_WorldRotation();
}

bool CameraTrack_Player::Can_OwnerRelative(const FCameraKey& key) const
{
    if (key.anchorSpace != ECinemaAnchorSpace::OwnerRelative)
    {
        return false;
    }

    return !_anchorTransform.expired();
}

void CameraTrack_Player::Evaluate(int32 frame)
{
    if (!_track || _track->keys.empty()) return;

    int32 prevIdx = 0, nextIdx = 0;
    if (!Find_KeySegment(frame, prevIdx, nextIdx))
        return;

    const auto& prevKey = _track->keys[prevIdx];
    const auto& nextKey = _track->keys[nextIdx];

    // t 계산 (0~1 사이 비율)
    float t = 0.f;
    int32 span = nextKey.frame - prevKey.frame;
    if (span > 0)
        t = static_cast<float>(frame - prevKey.frame) / static_cast<float>(span);

    t = std::clamp(t, 0.f, 1.f);

    // 이징 적용
    float easedT = Apply_Ease(t, nextKey.easeType);

    if (prevKey.anchorSpace == ECinemaAnchorSpace::OwnerRelative &&
        nextKey.anchorSpace == ECinemaAnchorSpace::OwnerRelative &&
        !_anchorTransform.expired())
    {
        auto anchor = _anchorTransform.lock();

        const Vec3 localPos = Lerp_Position(prevKey, nextKey, easedT);
        const Quat localRot = Lerp_Rotation(prevKey, nextKey, easedT);

        _currentPos = Vec3::Transform(localPos, anchor->Get_WorldMatrix());
        _currentRot = localRot * anchor->Get_WorldRotation();
    }
    else
    {
        const Vec3 prevWorldPos = Resolve_KeyWorldPosition(prevKey);
        const Vec3 nextWorldPos = Resolve_KeyWorldPosition(nextKey);

        const Quat prevWorldRot = Resolve_KeyWorldRotation(prevKey);
        const Quat nextWorldRot = Resolve_KeyWorldRotation(nextKey);

        _currentPos = prevWorldPos + (nextWorldPos - prevWorldPos) * easedT;
        _currentRot = Quat::Slerp(prevWorldRot, nextWorldRot, easedT);
    }

    _currentFovY = Lerp_FovY(prevKey, nextKey, easedT);
    _currentMode = (t < 1.f) ? prevKey.cameraMode : nextKey.cameraMode;

    // 현재 프레임의 카메라 키 상태를 별도로 스냅샷해 둔다.
    // Preview / Runtime 카메라가 Target / LookAt 전용 설정을 읽을 때 사용한다.
    _currentKey = (t < 1.f) ? prevKey : nextKey;
    _currentKey.cameraMode = _currentMode;
    _currentKey.position = _currentPos;
    _currentKey.rotation = _currentRot;
    _currentKey.fovY = _currentFovY;
    _currentKey.distance = prevKey.distance + (nextKey.distance - prevKey.distance) * easedT;
    _currentKey.targetOffset = prevKey.targetOffset + (nextKey.targetOffset - prevKey.targetOffset) * easedT;
    _currentKey.pitch = prevKey.pitch + (nextKey.pitch - prevKey.pitch) * easedT;
    _currentKey.yaw = prevKey.yaw + (nextKey.yaw - prevKey.yaw) * easedT;
}

Shared<CameraTrack_Player> CameraTrack_Player::Create()
{
    return make_shared<CameraTrack_Player>();
}

void CameraTrack_Player::Free()
{
    _track = nullptr;
    _anchorTransform.reset();
    OnFinished = nullptr;

    Base::Free();
}
