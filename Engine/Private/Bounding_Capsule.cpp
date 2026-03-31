#include "pch.h"
#include "Bounding_Capsule.h"
#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "DebugDraw.h"

Bounding_Capsule::Bounding_Capsule(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Bounding(device, context)
{
}

HRESULT Bounding_Capsule::Initialize(const FBoundingDesc& desc)
{
    CHECK_FAILED(Bounding::Initialize(desc), E_FAIL);

    const FBoundingCapsuleDesc& capsuleDesc = static_cast<const FBoundingCapsuleDesc&>(desc);

    _originRadius = capsuleDesc.radius;
    _originHalfHeight = capsuleDesc.halfHeight;

    _radius = _originRadius;
    _halfHeight = _originHalfHeight;

    _localCenter = desc.center + Vec3(0.f, _originHalfHeight + _originRadius, 0.f);

    Update_WorldCenter(Matrix::Identity);
    _worldMatrix = Matrix::CreateTranslation(_worldCenter);

    return S_OK;
}

void Bounding_Capsule::Update(const Matrix& worldMatrix)
{
    _worldMatrix = worldMatrix;

    Matrix tempMatrix = _worldMatrix;
    Vec3 scale, trans;
    Quat rot;

    tempMatrix.Decompose(scale, rot, trans);

    float maxScale = max(scale.x, max(scale.y, scale.z));

    _radius = _originRadius * maxScale;
    _halfHeight = _originHalfHeight;// *abs(scale.y);

    Update_WorldCenter(_worldMatrix);
}

void Bounding_Capsule::Update_WorldCenter(const Matrix& worldMatrix)
{
    _worldCenter = Vec3::Transform(_localCenter, worldMatrix);
}

bool Bounding_Capsule::Intersect(Bounding* otherBounding)
{
    EShape otherShape = otherBounding->Get_Shape();

    BoundingOrientedBox myProxy = Get_ProxyOBB();

    switch (otherShape)
    {
    case EShape::AABB:
        return myProxy.Intersects(static_cast<Bounding_AABB*>(otherBounding)->Get_AABB());
    case EShape::OBB:
        return myProxy.Intersects(static_cast<Bounding_OBB*>(otherBounding)->Get_OBB());
    case EShape::Sphere:
        return myProxy.Intersects(static_cast<Bounding_Sphere*>(otherBounding)->Get_Sphere());
    case EShape::Capsule:
        return myProxy.Intersects(static_cast<Bounding_Capsule*>(otherBounding)->Get_ProxyOBB());
    }

    return false;
}

BoundingOrientedBox Bounding_Capsule::Get_ProxyOBB() const
{
    BoundingOrientedBox proxy;
    proxy.Center = _worldCenter;

    proxy.Extents = Vec3(_radius, _halfHeight + _radius, _radius);

    Matrix tempMatrix = _worldMatrix;

    Vec3 scale, trans;
    Quat rot;
    tempMatrix.Decompose(scale, rot, trans);

    proxy.Orientation = rot;

    return proxy;
}

#ifdef _DEBUG
void Bounding_Capsule::Get_BasisVectors(Vec3& right, Vec3& up, Vec3& forward) const
{
    right = _worldMatrix.Right();
    right.Normalize();

    up = _worldMatrix.Up();
    up.Normalize();

    forward = _worldMatrix.Forward();
    forward.Normalize();
}

HRESULT Bounding_Capsule::Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color)
{
    Vec3 right, up, forward;
    Get_BasisVectors(right, up, forward);

    Vec3 topCenter = _worldCenter + (up * _halfHeight);
    Vec3 bottomCenter = _worldCenter - (up * _halfHeight);

    BoundingSphere topSphere(topCenter, _radius);
    BoundingSphere bottomSphere(bottomCenter, _radius);

    DX::Draw(batch, topSphere, color);
    DX::Draw(batch, bottomSphere, color);
    DX::DrawRing(batch, _worldCenter, right * _radius, forward * _radius, color);

    const Vec3 sideOffsets[4] =
    {
        right * _radius,
        -right * _radius,
        forward * _radius,
        -forward * _radius
    };

    for (const Vec3& sideOffset : sideOffsets)
    {
        DirectX::VertexPositionColor topVertex;
        topVertex.position = topCenter + sideOffset;
        topVertex.color = color;

        DirectX::VertexPositionColor bottomVertex;
        bottomVertex.position = bottomCenter + sideOffset;
        bottomVertex.color = color;

        batch->DrawLine(topVertex, bottomVertex);
    }

    return S_OK;
}
#endif

Shared<Bounding_Capsule> Bounding_Capsule::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const FBoundingCapsuleDesc& desc)
{
    auto instance = make_shared<Bounding_Capsule>(device, context);

    if (FAILED(instance->Initialize(desc)))
    {
        MSG_BOX("Failed to Create : Bounding_Capsule");

        return nullptr;
    }

    return instance;
}

void Bounding_Capsule::Free()
{
    Bounding::Free();
}
