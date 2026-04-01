#include "pch.h"
#include "Bounding_Sphere.h"

#include "Bounding_AABB.h"
#include "Bounding_Capsule.h"
#include "Bounding_OBB.h"
#include "GameInstance.h"
#include "DebugDraw.h"

Bounding_Sphere::Bounding_Sphere(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Bounding(device, context)
{
}

HRESULT Bounding_Sphere::Initialize(const FBoundingDesc& desc)
{
    CHECK_FAILED(Bounding::Initialize(desc), E_FAIL);

    const FBoundingSphereDesc& sphereDesc = static_cast<const FBoundingSphereDesc&>(desc);

    _originSphere = BoundingSphere(sphereDesc.center, sphereDesc.radius);
    _sphere = _originSphere;

    return S_OK;
}

void Bounding_Sphere::Update(const Matrix& worldMatrix)
{
    _originSphere.Transform(_sphere, worldMatrix);
}

bool Bounding_Sphere::Intersect(Bounding* otherBounding)
{
    EShape otherShape = otherBounding->Get_Shape();

    switch (otherShape)
    {
    case EShape::AABB:
    {
        auto otherAABB = static_cast<Bounding_AABB*>(otherBounding);
        return _sphere.Intersects(otherAABB->Get_AABB());
    }
    case EShape::OBB:
    {
        auto otherOBB = static_cast<Bounding_OBB*>(otherBounding);
        return _sphere.Intersects(otherOBB->Get_OBB());
    }
    case EShape::Sphere:
    {
        auto otherSphere = static_cast<Bounding_Sphere*>(otherBounding);
        return _sphere.Intersects(otherSphere->Get_Sphere());
    }
    case EShape::Capsule:
    {
        auto otherSphere = static_cast<Bounding_Capsule*>(otherBounding);
        return _sphere.Intersects(otherSphere->Get_ProxyOBB());
    }
    }
    return false;
}

#ifdef _DEBUG
HRESULT Bounding_Sphere::Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color)
{
    DX::Draw(batch, _sphere, color);

    return S_OK;
}
#endif

Shared<Bounding_Sphere> Bounding_Sphere::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const FBoundingSphereDesc& desc)
{
    auto instance = make_shared<Bounding_Sphere>(device, context);

    if (FAILED(instance->Initialize(desc)))
    {
        MSG_BOX("Failed to Create : Bounding_Sphere");

        return nullptr;
    }

    return instance;
}

void Bounding_Sphere::Free()
{
    Bounding::Free();
}
