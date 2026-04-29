#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Client)

class Skill_RasenShuriken : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_RasenShuriken)

public:
    explicit Skill_RasenShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_RasenShuriken(const Skill_RasenShuriken& rhs);
    virtual ~Skill_RasenShuriken() = default;
    
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

    // 충돌하거나 최대 비행 거리에 도달했을 때 나선수리검 폭발 히트 오브젝트를 한 번만 생성한다.
    void        Explode_RasenShuriken(const Vec3* overrideExplosionPosition = nullptr);

private:
    bool        _hasExploded = false;      // 나선수리검 종료 시 폭발 히트 오브젝트가 중복 생성되는 것을 막는 상태값이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
