#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bounding_Sphere final : public Bounding
{
public:
    struct FBoundingSphereDesc : public FBoundingDesc
    {
        float radius = 0.5f;
    };

public:
    explicit Bounding_Sphere(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Bounding_Sphere() = default;

public:
    HRESULT Initialize(const FBoundingDesc& desc) override;
    void    Update(const Matrix& worldMatrix) override;
    bool    Intersect(Bounding* otherBounding) override;

    EShape  Get_Shape() const override { return EShape::Sphere; }
    BoundingSphere& Get_OriginSphere() { return _originSphere; }

    const BoundingSphere& Get_Sphere() const { return _sphere; }

    bool Intersect_WithDepth(Bounding* otherBounding, Vec3& outNormal, float& outDepth) override;

#ifdef _DEBUG
    HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) override;
#endif

private:
    BoundingSphere  _originSphere;
    BoundingSphere  _sphere;

public:
    static Shared<Bounding_Sphere> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const FBoundingSphereDesc& desc);
    virtual void Free() override;

};

NS_END
