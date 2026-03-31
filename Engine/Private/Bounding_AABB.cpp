#include "pch.h"
#include "Bounding_AABB.h"

#include "Bounding_Capsule.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "GameInstance.h"
#include "DebugDraw.h"

Bounding_AABB::Bounding_AABB(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Bounding(device, context)
{
}

HRESULT Bounding_AABB::Initialize(const FBoundingDesc& desc)
{
    CHECK_FAILED(Bounding::Initialize(desc), E_FAIL);

    const FBoundingAABBDesc& aabbDesc = static_cast<const FBoundingAABBDesc>(desc);

    _originAABB = BoundingBox(aabbDesc.center, aabbDesc.extents);
    _aabb = _originAABB;

    return S_OK;
}

void Bounding_AABB::Update(const Matrix& worldMatrix)
{
    Matrix tempMatrix = worldMatrix;

    // AABB는 회전값 무시
    Vec3 scale, translation;
    Quat rotation;

    tempMatrix.Decompose(scale, rotation, translation);

    Matrix transform = Matrix::CreateScale(scale) * Matrix::CreateTranslation(translation);

    _originAABB.Transform(_aabb, transform);
}

bool Bounding_AABB::Intersect(Bounding* otherBounding)
{
    EShape otherShape = otherBounding->Get_Shape();

    switch (otherShape)
    {
    case EShape::AABB:
    {
        auto otherAABB = static_cast<Bounding_AABB*>(otherBounding);
        return _aabb.Intersects(otherAABB->Get_AABB());
    }
    case EShape::OBB:
    {
        auto otherOBB = static_cast<Bounding_OBB*>(otherBounding);
        return _aabb.Intersects(otherOBB->Get_OBB());
    }
    case EShape::Sphere:
    {
        auto otherSphere = static_cast<Bounding_Sphere*>(otherBounding);
        return _aabb.Intersects(otherSphere->Get_Sphere());
    }
    case EShape::Capsule:
    {
        auto otherCapsule = static_cast<Bounding_Capsule*>(otherBounding);
        return _aabb.Intersects(otherCapsule->Get_ProxyOBB());
    }
    }
    return false;
}

#ifdef _DEBUG
HRESULT Bounding_AABB::Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color)
{
    DX::Draw(batch, _aabb, color);

    return S_OK;
}
#endif

Shared<Bounding_AABB> Bounding_AABB::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const FBoundingAABBDesc& desc)
{
    auto instance = make_shared<Bounding_AABB>(device, context);

    if (FAILED(instance->Initialize(desc)))
    {
        MSG_BOX("Failed to Create : Bounding_AABB");

        return nullptr;
    }

    return instance;
}

void Bounding_AABB::Free()
{
    Bounding::Free();
}
