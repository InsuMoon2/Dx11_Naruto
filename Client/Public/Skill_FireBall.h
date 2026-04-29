#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Client)

class Skill_FireBall : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_FireBall)

public:
    explicit Skill_FireBall(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_FireBall(const Skill_FireBall& rhs);
    virtual ~Skill_FireBall() = default;
    
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

    // 충돌/거리 만료처럼 파이어볼이 종료될 때 히트 이펙트를 한 번만 생성한다.
    void        Explode_FireBall();
    // 폭발이 발생한 월드 위치 기준으로 주변 캐릭터들에게 범위 피해를 1회 적용한다.
    void        Apply_ExplosionAreaDamage(const Vec3& explosionCenter);

    void        Process_Hit(Character* hitted, GameObject* targetKey);

private:
    float       _explosionDamage = 20.f;   // 호화구 폭발에 휩쓸린 대상 하나당 적용할 최종 데미지다.
    float       _explosionRadius = 3.8f;   // 직격 한 명이 아니라 주변 대상들도 함께 맞도록 검사할 폭발 반경이다.
    float       _launchPower = 5.f;        // 호화구 폭발 시 몬스터가 더 멀리 밀려나도록 키운 수평 발사 힘이다.
    float       _launchUp = 3.f;           // 호화구 폭발 시 몬스터가 더 시원하게 뜨도록 키운 수직 띄우기 힘이다.
    bool        _hasExploded = false;      // 파이어볼 종료 시 히트 이펙트를 중복 생성하지 않도록 막는 상태값이다.


public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
