#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bounding_Capsule final : public Bounding
{
public:
    struct FBoundingCapsuleDesc : public FBoundingDesc
    {
        float radius = 0.5f;
        float halfHeight = 1.f; // 중심 기준 위/아래 길이
    };

public:
    explicit Bounding_Capsule(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Bounding_Capsule() = default;

public:
    HRESULT Initialize(const FBoundingDesc& desc) override;
    void    Update(const Matrix& worldMatrix) override;
    bool    Intersect(Bounding* otherBounding) override;

    EShape  Get_Shape() const override { return EShape::Capsule; }

    BoundingOrientedBox Get_ProxyOBB() const;


public:
    Vec3&  Get_LocalCenter()        { return _localCenter; }
    float& Get_OriginRadius()         { return _originRadius; }
    float& Get_OriginHalfHeight()     { return _originHalfHeight; }

    Vec3&  Get_LocalEuler()           { return _localEuler; }


    void   Set_LocalEuler(const Vec3& eulerDegrees);

    bool   Intersect_WithDepth(Bounding* otherBounding, Vec3& outNormal, float& outDepth) override;


#ifdef _DEBUG
    HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) override;
#endif

private:
    float _originRadius     = 0.5f;         
    float _originHalfHeight = 1.0f;         

    float _radius     = 0.5f;               
    float _halfHeight = 1.0f;               

    Vec3 _localCenter = Vec3::Zero;         
    Vec3 _worldCenter = Vec3::Zero;         

    Vec3 _localEuler     = Vec3::Zero;      
    Quat _localRotation  = Quat::Identity;  

    Matrix _worldMatrix = Matrix::Identity; 

private:
    void Update_WorldCenter(const Matrix& worldMatrix);

#ifdef _DEBUG
    void Get_BasisVectors(Vec3& right, Vec3& up, Vec3& forward) const;
#endif

public:
    static Shared<Bounding_Capsule> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const FBoundingCapsuleDesc& desc);
    virtual void Free() override;

};

NS_END
