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
    };

public:
    explicit SkillObject_Projectile(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject_Projectile(const SkillObject_Projectile& rhs);
    virtual ~SkillObject_Projectile() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

private:
    Shared<ProjectileComponent> _projectile;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
