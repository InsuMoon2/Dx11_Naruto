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

    /// Inspector에서 Euler 각도(degree)로 로컬 회전을 읽을 때 사용
    Vec3&  Get_LocalEuler()           { return _localEuler; }

    /// Euler 각도(degree) 설정 → 내부 _localRotation(Quat)을 자동 재계산
    /// Inspector changed 블록에서 호출된다
    void   Set_LocalEuler(const Vec3& eulerDegrees);

#ifdef _DEBUG
    HRESULT Render_Debug(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, Color color) override;
#endif

private:
    float _originRadius     = 0.5f;         ///< Inspector에서 편집된 원본 반지름
    float _originHalfHeight = 1.0f;         ///< Inspector에서 편집된 원본 반높이

    float _radius     = 0.5f;               ///< 스케일 적용 후 실제 반지름
    float _halfHeight = 1.0f;               ///< 스케일 적용 후 실제 반높이

    Vec3 _localCenter = Vec3::Zero;         ///< 오브젝트 로컬 공간 기준 캡슐 중심 오프셋
    Vec3 _worldCenter = Vec3::Zero;         ///< 매 Update마다 재계산되는 월드 중심 좌표

    Vec3 _localEuler     = Vec3::Zero;      ///< Inspector 편집용 로컬 회전 (degree, XYZ 순서)
    Quat _localRotation  = Quat::Identity;  ///< _localEuler에서 변환된 쿼터니언, 렌더/충돌에 실제 사용

    Matrix _worldMatrix = Matrix::Identity; ///< 부모 오브젝트 월드 행렬 (매 Update 갱신)

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
