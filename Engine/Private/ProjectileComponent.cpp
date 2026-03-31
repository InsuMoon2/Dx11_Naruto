#include "pch.h"
#include "ProjectileComponent.h"
#include "Transform.h"
#include "GameObject.h"

ProjectileComponent::ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

ProjectileComponent::ProjectileComponent(const ProjectileComponent& rhs)
    : Component(rhs)
    , _desc(rhs._desc)
{
}


HRESULT ProjectileComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT ProjectileComponent::Initialize(void* arg)
{
    if (arg)
    {
        auto* desc = static_cast<FProjectileDesc*>(arg);
        _desc = *desc;
    }

    return S_OK;
}

void ProjectileComponent::BeginPlay()
{
    Component::BeginPlay();

    _traveledDistance = 0.f;
    _elapsedTime = 0.f;

    Vec3 dir = _desc.direction;
    dir.Normalize();

    // 방향값 정규화해서 velocity 세팅
    _velocity = dir * _desc.speed;
}

void ProjectileComponent::Update_Projectile(float timeDelta)
{
    auto owner = Get_Owner();

    if (owner->Is_Destroy())
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;


    if (_desc.useGravity)
    {
        _velocity.y -= _desc.gravityScale * timeDelta;
    }

    // 이동량 계산
    Vec3 displacement = _velocity * timeDelta;
    float frameDistance = displacement.Length();

    // 위치 갱신
    Vec3 currentPos = transform->Get_LocalPosition();
    transform->Set_LocalPosition(currentPos + displacement);

    _traveledDistance += frameDistance;
    _elapsedTime += timeDelta;

    // 소멸 체크
    if (_desc.maxDistance > 0.f && _traveledDistance >= _desc.maxDistance)
    {
        owner->Set_Destroy(true);
    }
    else if (_desc.maxLifetime > 0.f && _elapsedTime >= _desc.maxLifetime)
    {
        owner->Set_Destroy(true);
    }
}

void ProjectileComponent::Set_Direction(const Vec3& dir)
{
    _desc.direction = dir;

    Vec3 normalized = dir;
    normalized.Normalize();

    _velocity = normalized * _desc.speed;
}

void ProjectileComponent::Setup(const FProjectileDesc& desc)
{
    _desc = desc;

    Vec3 dir = _desc.direction;

    dir.Normalize();
    _velocity = dir * _desc.speed;
}

Shared<ProjectileComponent> ProjectileComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<ProjectileComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : ProjectileComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> ProjectileComponent::Clone(void* arg)
{
    auto clone = make_shared<ProjectileComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : ProjectileComponent");

        return nullptr;
    }

    return clone;
}

void ProjectileComponent::Free()
{
    Component::Free();
}
