#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bounding_OBB final : public Bounding
{
public:
    struct FBoundingOBBDesc : public FBoundingDesc
    {
        Vec3 extents = Vec3(0.5f); 
        Vec3 radians = Vec3::Zero; 
    };

public:
    explicit Bounding_OBB(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Bounding_OBB() = default;

public:
    HRESULT Initialize(const FBoundingDesc& desc) override;
    void    Update(const Matrix& worldMatrix) override;
    bool    Intersect(Bounding* otherBounding) override;

    EShape  Get_Shape() const override { return EShape::OBB; }
    BoundingOrientedBox& Get_OriginOBB() { return _originalOBB; }

    const BoundingOrientedBox& Get_OBB() const { return _obb; }

#ifdef _DEBUG
    HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) override;
#endif

private:
    BoundingOrientedBox _originalOBB;
    BoundingOrientedBox _obb;

public:
    static Shared<Bounding_OBB> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const FBoundingOBBDesc& desc);
    virtual void Free() override;

};

NS_END
