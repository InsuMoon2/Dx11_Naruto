#pragma once

#include "Base.h"

NS_BEGIN(Engine)

struct FPhysXRaycastHit
{
    Vec3 position = Vec3::Zero;
    Vec3 normal = Vec3::Zero;
    float distance = 0.f;
    ECollisionProxyType proxyType = ECollisionProxyType::END;
    bool isBackFace = false; // ray 방향과 normal 방향이 같은 뒷면 hit인지 구분할 때 사용한다.
};

struct FPhysXContext;

class ENGINE_DLL PhysXMgr final : public Base
{
public:
    explicit PhysXMgr() = default;
    virtual ~PhysXMgr();

public:
    // PhysX foundation/physics/scene을 준비할 때 호출한다.
    HRESULT Initialize();

    // 충돌 전용 삼각형 메시를 PhysX static actor로 한 번 등록할 때 호출한다.
    bool Register_StaticTriangleMesh(
        const string& name,
        const vector<Vec3>& vertices,
        const vector<uint32>& indices,
        const Matrix& worldMatrix,
        ECollisionProxyType proxyType);

    // 벽/지형 감지용 raycast를 PhysX scene에 쏠 때 호출한다.
    bool Raycast(
        const Vec3& origin,
        const Vec3& direction,
        float distance,
        FPhysXRaycastHit& outHit,
        ECollisionProxyType requiredProxyType = ECollisionProxyType::END) const;

    // 레벨 전환 또는 충돌 데이터 재등록 전에 static geometry만 비울 때 호출한다.
    void Clear_StaticGeometry();

    bool Is_Ready() const;

public:
    static Unique<PhysXMgr> Create();
    void Free() override;

private:
    // PhysX SDK 타입을 public header에서 숨기기 위한 내부 context다.
    FPhysXContext* _context = nullptr;
};

NS_END
