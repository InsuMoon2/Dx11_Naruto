#include "pch.h"
#include "SkillObject.h"
#include "Model.h"
#include "Shader.h"
#include "Transform.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"

REGISTER_GAMEOBJECT(SkillObject, Protocol::OBJECT_TYPE_SKILL_OBJECT)

SkillObject::SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

SkillObject::SkillObject(const SkillObject& rhs)
    : GameObject(rhs)
{
}

HRESULT SkillObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT SkillObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    _lifetime = desc->lifetime;
    _ownerSkillId = desc->ownerSkillId;
    _elapsedTime = 0.f;

    if (_transformCom)
    {
        _transformCom->Set_LocalPosition(desc->spawnPosition);
        _transformCom->Set_LocalRotation(
            desc->spawnRotation.x,
            desc->spawnRotation.y,
            desc->spawnRotation.z);

        _transformCom->Set_LocalScale(desc->scale);
    }

    CHECK_FAILED(Ready_Components(*desc), E_FAIL);

    return S_OK;
}

void SkillObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void SkillObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_lifetime > 0.f)
    {
        _elapsedTime += timeDelta;

        if (_elapsedTime >= _lifetime)
        {
            Set_Destroy(true);
        }
    }
}

void SkillObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (!Is_Destroy() && _collider)
    {
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
    }
}

HRESULT SkillObject::Ready_Components(const FSkillObjectDesc& desc)
{
    if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_SPHERE)
    {
        Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
        sphereDesc.radius = desc.colliderRadius;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &sphereDesc), E_FAIL);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_AABB)
    {
        Bounding_AABB::FBoundingAABBDesc boxDesc{};
        boxDesc.extents = desc.colliderExtents;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &boxDesc), E_FAIL);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_OBB)
    {
        Bounding_OBB::FBoundingOBBDesc boxDesc{};
        boxDesc.extents = desc.colliderExtents;
        boxDesc.radians = desc.spawnRotation;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &boxDesc), E_FAIL);
    }
    else
    {
        return E_FAIL;
    }

    CHECK_NULL(_collider, E_FAIL);

    GAME->Add_Collider(_collider);

    return S_OK;
}


Shared<GameObject> SkillObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject::Clone(void* arg)
{
    auto clone = make_shared<SkillObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject");

        return nullptr;
    }

    return clone;
}

void SkillObject::Free()
{
    GameObject::Free();
}
