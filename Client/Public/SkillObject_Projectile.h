#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class ProjectileComponent;
NS_END

NS_BEGIN(Client)

class SkillObject_Projectile : public SkillObject
{
    GENERATED_BODY(SkillObject_Projectile)

public:
    struct FProjectileSkillDesc : public SkillObject::FSkillObjectDesc
    {
        Vec3    direction = Vec3::Forward;
        float   speed = 20.f;     
        float   maxDistance = 50.f;
        bool    useGravity = false;

        // true면 Launch 노티파이 호출 전까지 발사 ㄴㄴ
        bool    startAttached = false; 
    };

public:
    explicit SkillObject_Projectile(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject_Projectile(const SkillObject_Projectile& rhs);
    virtual ~SkillObject_Projectile() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

public:
    virtual void Launch(const Vec3& direction);
    bool    IsLaunched() const { return _isMoving; }


protected:
    Shared<ProjectileComponent> _projectile;

protected:
    // 발사 여부
    bool    _isMoving = false;

    float   _speed = 20.f;              
    float   _maxDistance = 50.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
