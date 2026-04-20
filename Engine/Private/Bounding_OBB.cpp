#include "pch.h"
#include "Bounding_OBB.h"
#include "Bounding_AABB.h"
#include "Bounding_Capsule.h"
#include "Bounding_Sphere.h"
#include "GameInstance.h"
#include "DebugDraw.h"

Bounding_OBB::Bounding_OBB(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Bounding(device, context)
{
}

HRESULT Bounding_OBB::Initialize(const FBoundingDesc& desc)
{
    CHECK_FAILED(Bounding::Initialize(desc), E_FAIL);

    const FBoundingOBBDesc& obbDesc = static_cast<const FBoundingOBBDesc&>(desc);

    Quat rotation = Quat::CreateFromYawPitchRoll(obbDesc.radians.y, obbDesc.radians.x, obbDesc.radians.z);

    _originalOBB = BoundingOrientedBox(obbDesc.center, obbDesc.extents, rotation);
    _obb = _originalOBB;

    return S_OK;
}

void Bounding_OBB::Update(const Matrix& worldMatrix)
{
    _originalOBB.Transform(_obb, worldMatrix);
}

bool Bounding_OBB::Intersect(Bounding* otherBounding)
{
    EShape otherShape = otherBounding->Get_Shape();

    switch (otherShape)
    {
    case EShape::AABB:
    {
        auto otherAABB = static_cast<Bounding_AABB*>(otherBounding);
        return _obb.Intersects(otherAABB->Get_AABB());
    }
    case EShape::OBB:
    {
        auto otherOBB = static_cast<Bounding_OBB*>(otherBounding);
        return _obb.Intersects(otherOBB->Get_OBB());
    }
    case EShape::Sphere:
    {
        auto otherSphere = static_cast<Bounding_Sphere*>(otherBounding);
        return _obb.Intersects(otherSphere->Get_Sphere());
    }
    case EShape::Capsule:
    {
        auto otherCapsule = static_cast<Bounding_Capsule*>(otherBounding);
        return _obb.Intersects(otherCapsule->Get_ProxyOBB());
    }
    }
    return false;
}

bool Bounding_OBB::Intersect_WithDepth(Bounding* otherBounding, Vec3& outNormal, float& outDepth)
{

    return true;
}

#ifdef _DEBUG
HRESULT Bounding_OBB::Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color)
{
    DX::Draw(batch, _obb, color);

    return S_OK;
}
#endif

Shared<Bounding_OBB> Bounding_OBB::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const FBoundingOBBDesc& desc)
{
    auto instance = make_shared<Bounding_OBB>(device, context);

    if (FAILED(instance->Initialize(desc)))
    {
        MSG_BOX("Failed to Create : Bounding_OBB");

        return nullptr;
    }

    return instance;
}

void Bounding_OBB::Free()
{
    Bounding::Free();
}
