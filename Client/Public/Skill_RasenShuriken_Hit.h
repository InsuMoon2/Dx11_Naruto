#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_RasenShuriken_Hit : public SkillObject
{
    GENERATED_BODY(Skill_RasenShuriken_Hit)

public:
    struct FHitDesc : public FSkillObjectDesc
    {
        Shared<GameObject> damageCauser = nullptr;
    };

public:
    explicit Skill_RasenShuriken_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_RasenShuriken_Hit(const Skill_RasenShuriken_Hit& rhs);
    virtual ~Skill_RasenShuriken_Hit() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    // 충둘된 놈이 실제로 타격 가능한지 판단
    Character*  Find_HitCharacter(Shared<Collider> other);

    // Hit 오브젝트가 생성될 때 지속 타격용 루프 사운드를 시작한다.
    void        Start_HitLoopSound();
    // 마지막 폭발 직전에 루프 사운드를 정리해서 Final 사운드와 겹치지 않게 한다.
    void        Stop_HitLoopSound();
    // 마지막 폭발 타이밍에 루프를 끊고 Final 사운드를 재생한다.
    void        Play_FinalBlastSound();
    void        Process_MultiHit(Character* hitted, GameObject* targetKey);
    void        Trigger_FinalHit(Character* character, GameObject* targetKey);

private:
    float       _baseScale = 1.0f;
    float       _maxScale = 2.4f;
    float       _scaleGrowSpeed = 2.8f;

    float       _midHitLaunchUp = 2.f;

    // 터질 때
    float       _finalBlastRadius = 2.5f;
    float       _finalBlastLaunchPower = 2.f;
    float       _finalBlastLaunchUp = 2.f;

    // Loop 사운드를 얼마 동안 유지한 뒤 Finish 사운드로 넘길지 정하는 재생 시간이다.
    float       _loopSoundDuration = 0.7f;
    // Hit 오브젝트가 시작된 뒤 Loop 사운드를 누적 재생한 시간이다.
    float       _loopSoundElapsedTime = 0.f;
    // RasenShuriken Hit가 유지되는 동안 재생할 지속 타격 루프 사운드 파일명이다.
    wstring     _loopSoundFile = L"RasenShuriken_Explosioning.wav";
    // 마지막 폭발 순간에 1회 재생할 마무리 사운드 파일명이다.
    wstring     _finalBlastSoundFile = L"RasenShuriken_Final.wav";
    // 루프 사운드를 이미 시작했는지 기록해서 중복 재생을 막는다.
    bool        _isHitLoopSoundPlaying = false;
    // Final 사운드를 이미 재생했는지 기록해서 마지막 히트에서 한 번만 울리게 한다.
    bool        _hasPlayedFinalBlastSound = false;

    // 터졌는지?
    bool        _isFinalBlast = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
