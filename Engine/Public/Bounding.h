#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bounding : public Base
{
public:
    struct FBoundingDesc
    {
        Vec3 center = Vec3::Zero;
    };

protected:
    explicit Bounding(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Bounding(const Bounding& rhs) = default;
    virtual ~Bounding() = default;

public:
    virtual HRESULT Initialize(const FBoundingDesc& desc);
    virtual void    Update(const Matrix& worldMatrix) = 0;

    virtual bool    Intersect(Bounding* otherBounding) = 0;

    virtual bool    Intersect_WithDepth(Bounding* otherBounding, Vec3& outNormal, float& outDepth) = 0;

#ifdef _DEBUG
    virtual HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) = 0;
#endif

public:
    virtual EShape  Get_Shape() const = 0;

protected:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    bool    _isColl = false;


public:
    virtual void Free() override;

};

NS_END
