#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Transform;

class ENGINE_DLL CameraTrack_Player : public Base
{
public:
    explicit CameraTrack_Player() = default;
    virtual ~CameraTrack_Player() = default;

public:
    // 트랙 바인딩
    void            Bind(const FCameraTrack* track);

    void            Set_AnchorTransform(Shared<Transform> anchorTransform);
    void            Clear_AnchorTransform();

    void            Play();
    void            Pause();
    void            Stop();

    bool            IsPlaying() const { return _isPlaying; }
    bool            IsFinished() const;

    void            Tick(float timeDelta);
    void            Seek(int32 frame);

    Vec3            Get_Position() const { return _currentPos; }
    Quat            Get_Rotation() const { return _currentRot; }
    float           Get_FovY() const { return _currentFovY; }
    int32           Get_CurrentFrame() const { return _currentFrame; }
    ECineCameraMode Get_CurrentMode() const { return _currentMode; }
    // 현재 프레임 평가 결과에 해당하는 카메라 키 설정 스냅샷이다.
    // Preview / Runtime 카메라가 Target / LookAt 전용 옵션을 읽을 때 사용한다.
    const FCameraKey& Get_CurrentKey() const { return _currentKey; }

    // 재생 완료
    function<void()> OnFinished;

private:
    Vec3  Lerp_Position(const FCameraKey& a, const FCameraKey& b, float t) const;
    Quat  Lerp_Rotation(const FCameraKey& a, const FCameraKey& b, float t) const;
    float Lerp_FovY(const FCameraKey& a, const FCameraKey& b, float time) const;

    Vec3  CatmullRom(const Vec3& p0, const Vec3& p1,
        const Vec3& p2, const Vec3& p3, float t) const;

    float Apply_Ease(float t, ECameraEaseType type) const;

    // 현재 프레임에 해당하는 키 구간 검색
    // prevIdx: 현재 프레임 직전 키, nextIdx: 직후 키
    bool Find_KeySegment(int32 frame, int32& prevIdx, int32& nextIdx) const;

    // 단일 키의 위치를 현재 anchor 기준 월드 위치로 해석한다.
    Vec3 Resolve_KeyWorldPosition(const FCameraKey& key) const;
    // 단일 키의 회전을 현재 anchor 기준 월드 회전으로 해석한다.
    Quat Resolve_KeyWorldRotation(const FCameraKey& key) const;

    // 현재 키가 owner-relative로 해석 가능한지 확인한다.
    bool Can_OwnerRelative(const FCameraKey& key) const;

    // 보간 결과를 _current 멤버에 저장
    void Evaluate(int32 frame);


private:
    const FCameraTrack* _track = nullptr;
    bool                _isPlaying = false;
    float               _playbackTimeSec = 0.f;
    int32               _currentFrame = 0;

    Weak<Transform>     _anchorTransform;

    // 보간 캐싱
    Vec3                _currentPos = Vec3::Zero;
    Quat                _currentRot = Quat::Identity;
    float               _currentFovY = XM_PIDIV4;
    ECineCameraMode     _currentMode = ECineCameraMode::Free;
    FCameraKey          _currentKey = {}; // 현재 프레임의 카메라 키 설정 스냅샷이다.

public:
    static Shared<CameraTrack_Player> Create();
    virtual void Free() override;

};

NS_END
