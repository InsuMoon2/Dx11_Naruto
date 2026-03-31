#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bounding_AABB final : public Bounding
{
public:
    struct FBoundingAABBDesc : public FBoundingDesc
    {
        Vec3 extents = Vec3(0.5f); 
    };

public:
    explicit Bounding_AABB(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Bounding_AABB() = default;

public:
    HRESULT Initialize(const FBoundingDesc& desc) override;
    void    Update(const Matrix& worldMatrix) override;
    bool    Intersect(Bounding* otherBounding) override;

    EShape  Get_Shape() const override { return EShape::AABB; }

    const BoundingBox& Get_AABB() const { return _aabb; }

#ifdef _DEBUG
    HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) override;
#endif

private:
    BoundingBox _originAABB;
    BoundingBox _aabb;

public:
    static Shared<Bounding_AABB> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const FBoundingAABBDesc& desc);
    virtual void Free() override;

};

NS_END
