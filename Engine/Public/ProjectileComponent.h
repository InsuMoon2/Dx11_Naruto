#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL ProjectileComponent : public Component
{
    GENERATED_COMPONENT(ProjectileComponent, Protocol::COMPONENT_TYPE_PROJECTILE)

public:
    struct FProjectileDesc
    {
        Vec3    direction = Vec3::Forward;
        float   speed = 20.f;

        float   maxDistance = 50.f;
        float   maxLifetime = 0.f;

        bool    useGravity = false;       
        float   gravityScale = 9.8f;    // 중력 가속도      
    };

public:
    explicit ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit ProjectileComponent(const ProjectileComponent& rhs);
    virtual ~ProjectileComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Update_Projectile(float timeDelta);

public:
    // 지금까지 이동한 총 거리
    float   Get_TraveledDistance() const { return _traveledDistance; }

    void    Set_Direction(const Vec3& dir);
    void    Set_Speed(float speed) { _desc.speed = speed; }
    void    Setup(const FProjectileDesc& desc);

private:
    FProjectileDesc _desc;

    float   _traveledDistance = 0.f;    // 누적 이동 거리
    float   _elapsedTime = 0.f;         // 경과 시간

    Vec3    _velocity = Vec3::Zero;

public:
    static Shared<ProjectileComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
