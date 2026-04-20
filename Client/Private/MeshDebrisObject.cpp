#include "pch.h"
#include "MeshDebrisObject.h"

#include "GameObject_Factory.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(MeshDebrisObject, Protocol::OBJECT_TYPE_MESH_DEBRIS, "EffectSpawn");

MeshDebrisObject::MeshDebrisObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

MeshDebrisObject::MeshDebrisObject(const MeshDebrisObject& rhs)
    : GameObject(rhs)
    , _effectCom(rhs._effectCom)
    , _effectAssetName(rhs._effectAssetName)
    , _velocity(rhs._velocity)
    , _gravity(rhs._gravity)
    , _angularVelocityDeg(rhs._angularVelocityDeg)
    , _lifetime(rhs._lifetime)
    , _elapsedTime(0.f)
    , _groundY(rhs._groundY)
    , _destroyOnGroundHit(rhs._destroyOnGroundHit)
    , _stopOnGroundHit(rhs._stopOnGroundHit)
    , _hasHitGround(false)
{
}

HRESULT MeshDebrisObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT MeshDebrisObject::Initialize(void* arg)
{
    auto* desc = static_cast<FMeshDebrisDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    _effectAssetName = desc->effectAssetName;
    _velocity = desc->initialVelocity;
    _gravity = desc->gravity;
    _angularVelocityDeg = desc->angularVelocityDeg;
    _lifetime = desc->lifetime;
    _groundY = desc->groundY;
    _destroyOnGroundHit = desc->destroyOnGroundHit;
    _stopOnGroundHit = desc->stopOnGroundHit;

    if (_transformCom)
    {
        _transformCom->Set_WorldPosition(desc->position);

        _transformCom->Set_LocalRotation(
            desc->spawnRotation.x,
            desc->spawnRotation.y,
            desc->spawnRotation.z);

        _transformCom->Set_LocalScale(desc->spawnScale);
    }

    CHECK_FAILED(Ready_Components(*desc), E_FAIL);

    _elapsedTime = 0.f;
    _hasHitGround = false;

    return S_OK;
}

void MeshDebrisObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (Is_Destroy())
        return;

    _elapsedTime += timeDelta;

    if (_lifetime > 0.f && _elapsedTime >= _lifetime)
    {
        Set_Destroy(true);
        return;
    }

    if (_hasHitGround && _stopOnGroundHit)
    {
        if (_effectCom)
            _effectCom->Update(timeDelta);
        return;
    }

    if (_transformCom)
    {
        _velocity.y += _gravity * timeDelta;

        _transformCom->Add_WorldOffset(_velocity * timeDelta);

        if (abs(_angularVelocityDeg.x) > FLT_EPSILON)
            _transformCom->Rotate_Axis(Vec3::Right, _angularVelocityDeg.x * timeDelta);

        if (abs(_angularVelocityDeg.y) > FLT_EPSILON)
            _transformCom->Rotate_Axis(Vec3::Up, _angularVelocityDeg.y * timeDelta);

        if (abs(_angularVelocityDeg.z) > FLT_EPSILON)
            _transformCom->Rotate_Axis(Vec3::Forward, _angularVelocityDeg.z * timeDelta);
    }

    Resolve_GroundHit();

    if (_effectCom)
        _effectCom->Update(timeDelta);
}

void MeshDebrisObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    if (_effectCom)
        _effectCom->Late_Update(timeDelta);
}

HRESULT MeshDebrisObject::Render()
{
    GameObject::Render();

    return S_OK;
}

HRESULT MeshDebrisObject::Ready_Components(const FMeshDebrisDesc& desc)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);

    CHECK_NULL(_effectCom, E_FAIL);

    EffectComponent::FPlayDesc playDesc{};

    playDesc.effectAssetName = desc.effectAssetName;

    playDesc.localPosition = desc.effectLocalPosition;
    playDesc.localRotation = desc.effectLocalRotation;

    playDesc.localScale = desc.effectLocalScale;
    playDesc.loopOverride = false;

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void MeshDebrisObject::Resolve_GroundHit()
{
    if (!_transformCom || _hasHitGround)
        return;

    const Vec3 currentPos = _transformCom->Get_WorldPosition();
    if (currentPos.y > _groundY)
        return;

    _hasHitGround = true;

    Vec3 clampedPos = currentPos;
    clampedPos.y = _groundY;
    _transformCom->Set_WorldPosition(clampedPos);

    if (_stopOnGroundHit)
    {
        _velocity = Vec3::Zero;
    }

    if (_destroyOnGroundHit)
    {
        Set_Destroy(true);
    }
}

Shared<GameObject> MeshDebrisObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MeshDebrisObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : MeshDebrisObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> MeshDebrisObject::Clone(void* arg)
{
    auto clone = make_shared<MeshDebrisObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : MeshDebrisObject");
        return nullptr;
    }

    return clone;
}

void MeshDebrisObject::Free()
{
    GameObject::Free();
}
