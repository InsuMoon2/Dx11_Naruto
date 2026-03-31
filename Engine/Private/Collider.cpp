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
#ifdef _DEBUG
    , _batch(rhs._batch)
    , _effect(rhs._effect)
    , _inputLayout(rhs._inputLayout)
#endif
{
}

HRESULT Collider::Initialize_Prototype(EShape shape)
{
    _shape = shape;

#ifdef _DEBUG
    _batch = make_shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(_context.Get());
    _effect = make_shared<DirectX::BasicEffect>(_device.Get());
    _effect->SetVertexColorEnabled(true);

    const void* pShaderByteCode = nullptr;
    size_t iShaderByteCodeLength = 0;
    _effect->GetVertexShaderBytecode(&pShaderByteCode, &iShaderByteCodeLength);

    if (FAILED(_device->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,       
        DirectX::VertexPositionColor::InputElementCount,   
        pShaderByteCode,
        iShaderByteCodeLength,
        &_inputLayout)))
    {

        return E_FAIL;
    }
#endif

    return S_OK;
}

HRESULT Collider::Initialize(void* arg)
{
    if (!arg) return E_FAIL;

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

bool Collider::Is_Overlapping(Shared<Collider> other) const
{
    return _overlapSet.find(other) != _overlapSet.end();
}

void Collider::Set_CollisionPreset(Collision_Preset preset)
{
    const FCollision_Preset_Data& data = Get_PresetData(preset);

    _channel = data.channel;
    _collisionMask = data.collisionMask;
}

#ifdef _DEBUG
HRESULT Collider::Render_Debug()
{
    if (!_bounding || !_isActive)
        return S_OK;

    _effect->SetWorld(Matrix::Identity);
    _effect->SetView(*GAME->Get_Transform(ETransformState::View));
    _effect->SetProjection(*GAME->Get_Transform(ETransformState::Proj));

    _context->IASetInputLayout(_inputLayout.Get());
    _effect->Apply(_context.Get());

    _batch->Begin();

    // 충돌 안하면 초록색 -> 충돌 시 빨간색
    Color color = _isColl ? Color(1.f, 0.f, 0.f, 1.f) : Color(0.f, 1.f, 0.f, 1.f);

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
