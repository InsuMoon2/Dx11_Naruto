#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Kamui final : public SkillObject
{
    GENERATED_BODY(Skill_Kamui)

public:
    explicit Skill_Kamui(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Kamui(const Skill_Kamui& rhs);
    virtual ~Skill_Kamui() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    // 카무이 overlap 첫 접촉 시 대상 위치에 재생할 전용 히트 이펙트 이름이다.
    void        Spawn_KamuiHitEffect(Character* hitted);
    Character*  Find_HitCharacter(Shared<Collider> other);
    void        Process_MultiHit(Character* hitted, GameObject* targetKey);

private:
    float _baseDamage = 6.f; 
    float _finalDamage = 14.f; 

    float _midHitLaunchForce = 0.f;
    float _midHitLaunchUp = 0.f; 

    float _finalHitLaunchForce = 8.f;
    float _finalHitLaunchUp = 2.f;

    int32 _currentHitStep = 0; 
    umap<GameObject*, int32> _lastAppliedStepByTarget;
    string _hitEffectAssetName = "Kamui_HitParticle"; // 카무이 적중 시 OnBeginOverlap에서 재생할 충돌 이펙트 에셋 이름이다.
    wstring _finalExplosionSoundFile = L"Kamui_Explosion.wav"; // 카무이 마지막 폭발 히트 순간 1회 재생할 마무리 사운드 파일명이다.
    wstring _tickHitSoundFile = L"KamuiRasenSHurikenHit_Small.wav"; // 카무이 지속 흡입 구간에서 틱 데미지가 들어갈 때마다 재생할 타격 사운드 파일명이다.
    bool _hasPlayedFinalExplosionSound = false; // 마지막 폭발 사운드가 여러 대상 겹침으로 중복 재생되지 않게 막는 상태값이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
