#include "pch.h"
#include "Collider.h"
#include "GameInstance.h"

#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "Bounding_Capsule.h"
#include "Collision_Define.h"

Collider::Collider(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Collider::Collider(const Collider& rhs)
    : Component(rhs)
    , _shape(rhs._shape)
{
}

HRESULT Collider::Initialize_Prototype(EShape shape)
{
    _shape = shape;

#ifdef _DEBUG
    CHECK_FAILED(Ready_DebugRenderResources(), E_FAIL);
#endif

    return S_OK;
}

HRESULT Collider::Initialize(void* arg)
{
    if (!arg) return E_FAIL;

#ifdef _DEBUG
    /* clone된 Collider도 prototype과 디버그 렌더 자원을 공유하지 않도록 초기화 시점에 별도 생성한다. */
    if (!_batch || !_effect || !_inputLayout)
    {
        CHECK_FAILED(Ready_DebugRenderResources(), E_FAIL);
    }
#endif

    switch (_shape)
    {
    case EShape::AABB:
        _bounding = Bounding_AABB::Create(_device, _context, *static_cast<Bounding_AABB::FBoundingAABBDesc*>(arg));
        break;
    case EShape::OBB:
        _bounding = Bounding_OBB::Create(_device, _context, *static_cast<Bounding_OBB::FBoundingOBBDesc*>(arg));
        break;
    case EShape::Sphere:
        _bounding = Bounding_Sphere::Create(_device, _context, *static_cast<Bounding_Sphere::FBoundingSphereDesc*>(arg));
        break;
    case EShape::Capsule:
        _bounding = Bounding_Capsule::Create(_device, _context, *static_cast<Bounding_Capsule::FBoundingCapsuleDesc*>(arg));
        break;
    }

    if (!_bounding)
        return E_FAIL;

    return S_OK;
}

#ifdef _DEBUG
HRESULT Collider::Ready_DebugRenderResources()
{
    _batch = make_shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(_context.Get());
    _effect = make_shared<DirectX::BasicEffect>(_device.Get());
    CHECK_NULL(_batch, E_FAIL);
    CHECK_NULL(_effect, E_FAIL);

    _effect->SetVertexColorEnabled(true);

    const void* pShaderByteCode = nullptr;
    size_t iShaderByteCodeLength = 0;
    _effect->GetVertexShaderBytecode(&pShaderByteCode, &iShaderByteCodeLength);

    _inputLayout.Reset();

    if (FAILED(_device->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        pShaderByteCode,
        iShaderByteCodeLength,
        &_inputLayout)))
    {
        return E_FAIL;
    }

    return S_OK;
}
#endif


void Collider::Update_Collider(const Matrix& worldMatrix)
{
    if (_bounding)
    {
        _bounding->Update(worldMatrix);
    }
}

bool Collider::Intersect(Shared<Collider> target)
{
    if (!_bounding || !target->Get_Bounding())
        return false;

    return _bounding->Intersect(target->Get_Bounding().get());
}

bool Collider::Intersect_WithDepth(Shared<Collider> target, Vec3& outNormal, float& outDepth)
{
    if (target == nullptr || _bounding == nullptr || target->Get_Bounding() == nullptr)
        return false;

    return _bounding->Intersect_WithDepth(target->Get_Bounding().get(), outNormal, outDepth);
}

bool Collider::Is_Overlapping(Shared<Collider> other) const
{
    return _overlapSet.contains(other);
}

void Collider::Set_IsActive(bool active)
{
    if (_isActive == active)
        return;

    if (active == false)
    {
        auto myShared = static_pointer_cast<Collider>(GetSharedPtr());

        vector<Shared<Collider>> overlappedColliders;
        overlappedColliders.reserve(_overlapSet.size());

        for (const auto& weakOther : _overlapSet)
        {
            auto other = weakOther.lock();
            if (other)
                overlappedColliders.push_back(other);
        }

        for (auto other : overlappedColliders)
        {
            other->Remove_Overlap(myShared);
            Remove_Overlap(other);
        }

        _isColl = false;
    }

    _isActive = active;
}

void Collider::Set_CollisionPreset(Collision_Preset preset)
{
    _preset = preset;

    const FCollision_Preset_Data& data = Get_PresetData(preset);

    _channel = data.channel;
    _overlapMask = data.overlapMask;
    _blockMask = data.blockMask;
}

json Collider::To_Json() const
{
    json j = Component::To_Json();

    j["shape"] = magic_enum::enum_name(_shape);
    j["channel"] = magic_enum::enum_name(_channel);
    j["collision_preset"] = magic_enum::enum_name(_preset);

    if (_preset == Collision_Preset::Custom)
    {
        j["overlap_mask"] = _overlapMask;
        j["block_mask"] = _blockMask;
    }

    j["is_active"] = _isActive;

    if (!_bounding)
        return j;

    json b;

    switch (_shape)
    {
    case EShape::AABB:
    {
        auto pAABB = static_pointer_cast<Bounding_AABB>(_bounding);
        BoundingBox& origin = pAABB->Get_OriginAABB();
        b["center"] = { origin.Center.x,  origin.Center.y,  origin.Center.z };
        b["extents"] = { origin.Extents.x, origin.Extents.y, origin.Extents.z };
        break;
    }
    case EShape::OBB:
    {
        auto pOBB = static_pointer_cast<Bounding_OBB>(_bounding);
        BoundingOrientedBox& origin = pOBB->Get_OriginOBB();
        b["center"] = { origin.Center.x,  origin.Center.y,  origin.Center.z };
        b["extents"] = { origin.Extents.x, origin.Extents.y, origin.Extents.z };
        break;
    }
    case EShape::Sphere:
    {
        auto pSphere = static_pointer_cast<Bounding_Sphere>(_bounding);
        BoundingSphere& origin = pSphere->Get_OriginSphere();
        b["center"] = { origin.Center.x, origin.Center.y, origin.Center.z };
        b["radius"] = origin.Radius;
        break;
    }
    case EShape::Capsule:
    {
        auto pCapsule = static_pointer_cast<Bounding_Capsule>(_bounding);
        Vec3& c = pCapsule->Get_LocalCenter();
        Vec3& e = pCapsule->Get_LocalEuler();
        b["local_center"] = { c.x, c.y, c.z };
        b["origin_radius"] = pCapsule->Get_OriginRadius();
        b["origin_half_height"] = pCapsule->Get_OriginHalfHeight();
        b["local_euler"] = { e.x, e.y, e.z };
        break;
    }
    }

    j["bounding"] = b;
    return j;
}

void Collider::From_Json(const json& data)
{
    Component::From_Json(data);
    if (data.contains("is_active"))
        _isActive = data["is_active"].get<bool>();

    if (data.contains("collision_preset"))
    {
        auto result
            = magic_enum::enum_cast<Collision_Preset>(data["collision_preset"].get<string>());

        if (result.has_value())
            Set_CollisionPreset(result.value()); 
    }
    else
    {
        // 레거시 코드 호환용
        if (data.contains("collision_mask"))
            _overlapMask = data["collision_mask"].get<uint32>();

        if (data.contains("overlap_mask"))
            _overlapMask = data["overlap_mask"].get<uint32>();

        if (data.contains("block_mask"))
            _blockMask = data["block_mask"].get<uint32>();
    }

    if (data.contains("channel"))
    {
        auto result = magic_enum::enum_cast<Collision_Channel>(data["channel"].get<string>());
        if (result.has_value())
            _channel = result.value();
    }
    if (!_bounding || !data.contains("bounding"))
        return;
    const json& b = data["bounding"];

    switch (_shape)
    {
    case EShape::AABB:
    {
        auto pAABB = static_pointer_cast<Bounding_AABB>(_bounding);
        BoundingBox& origin = pAABB->Get_OriginAABB();
        if (b.contains("center"))
        {
            origin.Center.x = b["center"][0].get<float>();
            origin.Center.y = b["center"][1].get<float>();
            origin.Center.z = b["center"][2].get<float>();
        }
        if (b.contains("extents"))
        {
            origin.Extents.x = b["extents"][0].get<float>();
            origin.Extents.y = b["extents"][1].get<float>();
            origin.Extents.z = b["extents"][2].get<float>();
        }
        break;
    }
    case EShape::OBB:
    {
        auto pOBB = static_pointer_cast<Bounding_OBB>(_bounding);
        BoundingOrientedBox& origin = pOBB->Get_OriginOBB();
        if (b.contains("center"))
        {
            origin.Center.x = b["center"][0].get<float>();
            origin.Center.y = b["center"][1].get<float>();
            origin.Center.z = b["center"][2].get<float>();
        }
        if (b.contains("extents"))
        {
            origin.Extents.x = b["extents"][0].get<float>();
            origin.Extents.y = b["extents"][1].get<float>();
            origin.Extents.z = b["extents"][2].get<float>();
        }
        break;
    }
    case EShape::Sphere:
    {
        auto pSphere = static_pointer_cast<Bounding_Sphere>(_bounding);
        BoundingSphere& origin = pSphere->Get_OriginSphere();
        if (b.contains("center"))
        {
            origin.Center.x = b["center"][0].get<float>();
            origin.Center.y = b["center"][1].get<float>();
            origin.Center.z = b["center"][2].get<float>();
        }
        if (b.contains("radius"))
            origin.Radius = b["radius"].get<float>();
        break;
    }
    case EShape::Capsule:
    {
        auto pCapsule = static_pointer_cast<Bounding_Capsule>(_bounding);
        if (b.contains("local_center"))
        {
            Vec3& c = pCapsule->Get_LocalCenter();
            c.x = b["local_center"][0].get<float>();
            c.y = b["local_center"][1].get<float>();
            c.z = b["local_center"][2].get<float>();
        }
        if (b.contains("origin_radius"))
            pCapsule->Get_OriginRadius() = b["origin_radius"].get<float>();
        if (b.contains("origin_half_height"))
            pCapsule->Get_OriginHalfHeight() = b["origin_half_height"].get<float>();
        if (b.contains("local_euler"))
        {
            Vec3 euler(
                b["local_euler"][0].get<float>(),
                b["local_euler"][1].get<float>(),
                b["local_euler"][2].get<float>()
            );
            pCapsule->Set_LocalEuler(euler);
        }
        break;
    }
    }
}

#ifdef _DEBUG
HRESULT Collider::Render_Debug()
{
    if (!_bounding)
        return S_OK;

    if (!_effect || !_batch || !_inputLayout)
        return S_OK;

    _effect->SetWorld(Matrix::Identity);

    const Matrix view = *GAME->Get_Transform(ETransformState::View);
    const Matrix proj = *GAME->Get_Transform(ETransformState::Proj);

    _effect->SetView(view);
    _effect->SetProjection(proj);

    _context->IASetInputLayout(_inputLayout.Get());
    _effect->Apply(_context.Get());

    _batch->Begin();

    // 충돌 안하면 초록색 -> 충돌 시 빨간색
    Color color;

    if (!_isActive)     // 렌더
        color = Color(0.5f, 0.5f, 0.5f, 1.f); 
    else if (_isColl)   // 충돌
        color = Color(1.f, 0.f, 0.f, 1.f);     
    else
        color = Color(0.f, 1.f, 0.f, 1.f);     

    _bounding->Render_Debug(_batch.get(), color);
    _batch->End();

    return S_OK;
}
#endif

Shared<Collider> Collider::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EShape shape)
{
    auto instance = make_shared<Collider>(device, context);

    if (FAILED(instance->Initialize_Prototype(shape)))
    {
        MSG_BOX("Failed to Create : Collider");

        return nullptr;
    }

    return instance;
}

Shared<Component> Collider::Clone(void* arg)
{
    auto clone = make_shared<Collider>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Collider");

        return nullptr;
    }

    return clone;
}

void Collider::Free()
{
    Component::Free();

    _bounding = nullptr;
    _overlapSet.clear();

#ifdef _DEBUG
    _batch.reset();
    _effect.reset();
    _inputLayout.Reset();
#endif
}
