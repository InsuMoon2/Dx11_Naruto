#include "pch.h"
#include "PhysXMgr.h"

#define PX_PHYSX_STATIC_LIB
#pragma push_macro("new")
#undef new
#include <physx/PxPhysicsAPI.h>
#include <physx/cooking/PxCooking.h>
#pragma pop_macro("new")

using namespace physx;

NS_BEGIN(Engine)

struct FPhysXContext
{
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;
    PxTolerancesScale scale;
    PxFoundation* foundation = nullptr;
    PxPhysics* physics = nullptr;
    PxDefaultCpuDispatcher* dispatcher = nullptr;
    PxScene* scene = nullptr;
    PxMaterial* material = nullptr;
    vector<PxRigidStatic*> staticActors;
    vector<PxTriangleMesh*> triangleMeshes;
};

// Engine Vec3를 PhysX API 입력값으로 넘길 때 사용하는 변환 helper다.
static PxVec3 ToPxVec3(const Vec3& value)
{
    return PxVec3(value.x, value.y, value.z);
}

// PhysX hit 결과를 Engine 좌표 타입으로 되돌릴 때 사용하는 변환 helper다.
static Vec3 ToEngineVec3(const PxVec3& value)
{
    return Vec3(value.x, value.y, value.z);
}

class FPhysXProxyTypeFilter final : public PxQueryFilterCallback
{
public:
    explicit FPhysXProxyTypeFilter(ECollisionProxyType requiredProxyType)
        : _requiredProxyType(requiredProxyType)
    {
    }

    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags& queryFlags) override
    {
        UNREFERENCED_PARAMETER(filterData);
        UNREFERENCED_PARAMETER(actor);
        UNREFERENCED_PARAMETER(queryFlags);

        if (_requiredProxyType == ECollisionProxyType::END)
            return PxQueryHitType::eBLOCK;

        if (!shape)
            return PxQueryHitType::eNONE;

        const PxFilterData shapeFilterData = shape->getQueryFilterData();

        return shapeFilterData.word0 == static_cast<PxU32>(_requiredProxyType)
            ? PxQueryHitType::eBLOCK
            : PxQueryHitType::eNONE;
    }

    PxQueryHitType::Enum postFilter(
        const PxFilterData& filterData,
        const PxQueryHit& hit,
        const PxShape* shape,
        const PxRigidActor* actor) override
    {
        UNREFERENCED_PARAMETER(filterData);
        UNREFERENCED_PARAMETER(hit);
        UNREFERENCED_PARAMETER(shape);
        UNREFERENCED_PARAMETER(actor);
        return PxQueryHitType::eBLOCK;
    }

private:
    ECollisionProxyType _requiredProxyType = ECollisionProxyType::END;
};

PhysXMgr::~PhysXMgr()
{
    Free();
}

HRESULT PhysXMgr::Initialize()
{
    // Engine 초기화 중 PhysX scene을 한 번 준비한다.
    if (_context)
        return S_OK;

    _context = new FPhysXContext();

    _context->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _context->allocator, _context->errorCallback);
    CHECK_NULL(_context->foundation, E_FAIL);

    _context->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_context->foundation, _context->scale, true);
    CHECK_NULL(_context->physics, E_FAIL);

    PxSceneDesc sceneDesc(_context->scale);
    sceneDesc.gravity = PxVec3(0.f, -9.8f, 0.f);

    _context->dispatcher = PxDefaultCpuDispatcherCreate(2);
    CHECK_NULL(_context->dispatcher, E_FAIL);

    sceneDesc.cpuDispatcher = _context->dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    _context->scene = _context->physics->createScene(sceneDesc);
    CHECK_NULL(_context->scene, E_FAIL);

    _context->material = _context->physics->createMaterial(0.5f, 0.5f, 0.1f);
    CHECK_NULL(_context->material, E_FAIL);

    return S_OK;
}

bool PhysXMgr::Register_StaticTriangleMesh(
    const string& name,
    const vector<Vec3>& vertices,
    const vector<uint32>& indices,
    const Matrix& worldMatrix,
    ECollisionProxyType proxyType)
{
    // 레벨 충돌 메시를 world-space triangle mesh로 cook해서 static actor로 등록한다.
    if (!Is_Ready())
        return false;

    if (vertices.empty() || indices.size() < 3 || indices.size() % 3 != 0)
        return false;

    vector<PxVec3> cookedVertices;
    cookedVertices.reserve(vertices.size());

    for (const Vec3& vertex : vertices)
    {
        const Vec3 worldVertex = Vec3::Transform(vertex, worldMatrix);
        cookedVertices.push_back(ToPxVec3(worldVertex));
    }

    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = static_cast<PxU32>(cookedVertices.size());
    meshDesc.points.stride = sizeof(PxVec3);
    meshDesc.points.data = cookedVertices.data();
    meshDesc.triangles.count = static_cast<PxU32>(indices.size() / 3);
    meshDesc.triangles.stride = sizeof(uint32) * 3;
    meshDesc.triangles.data = indices.data();

    PxTriangleMeshCookingResult::Enum cookingResult = PxTriangleMeshCookingResult::eFAILURE;
    PxCookingParams cookingParams(_context->scale);
    cookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
    cookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

    PxTriangleMesh* triangleMesh = PxCreateTriangleMesh(
        cookingParams,
        meshDesc,
        _context->physics->getPhysicsInsertionCallback(),
        &cookingResult);

    if (!triangleMesh)
    {
        LOG_WARN("PhysX triangle mesh creation failed: {}", name);
        return false;
    }

    PxRigidStatic* actor = _context->physics->createRigidStatic(PxTransform(PxIdentity));
    if (!actor)
    {
        triangleMesh->release();
        LOG_WARN("PhysX static actor creation failed: {}", name);
        return false;
    }

    PxShape* shape = _context->physics->createShape(PxTriangleMeshGeometry(triangleMesh), *_context->material);
    if (!shape)
    {
        actor->release();
        triangleMesh->release();
        LOG_WARN("PhysX shape creation failed: {}", name);
        return false;
    }

    const PxU32 proxyTypeValue = static_cast<PxU32>(proxyType);
    shape->setQueryFilterData(PxFilterData(proxyTypeValue, 0, 0, 0));
    shape->setSimulationFilterData(PxFilterData(proxyTypeValue, 0, 0, 0));
    actor->attachShape(*shape);
    shape->release();

    _context->scene->addActor(*actor);
    _context->staticActors.push_back(actor);
    _context->triangleMeshes.push_back(triangleMesh);

    return true;
}

bool PhysXMgr::Raycast(
    const Vec3& origin,
    const Vec3& direction,
    float distance,
    FPhysXRaycastHit& outHit,
    ECollisionProxyType requiredProxyType) const
{
    // MovementComponent가 PhysX 기반 벽/지형 감지를 시험할 수 있도록 scene raycast를 감싼다.
    if (!Is_Ready())
        return false;

    if (distance <= 0.f || direction.LengthSquared() <= 0.000001f)
        return false;

    Vec3 normalizedDirection = direction;
    normalizedDirection.Normalize();

    PxRaycastBuffer hitBuffer;
    const PxHitFlags hitFlags =
        PxHitFlag::ePOSITION |
        PxHitFlag::eNORMAL |
        PxHitFlag::eMESH_BOTH_SIDES;
    PxQueryFilterData filterData;
    filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;
    FPhysXProxyTypeFilter proxyTypeFilter(requiredProxyType);

    const bool hit = _context->scene->raycast(
        ToPxVec3(origin),
        ToPxVec3(normalizedDirection),
        distance,
        hitBuffer,
        hitFlags,
        filterData,
        &proxyTypeFilter);

    if (!hit || !hitBuffer.hasBlock)
        return false;

    outHit.position = ToEngineVec3(hitBuffer.block.position);
    outHit.normal = ToEngineVec3(hitBuffer.block.normal);
    outHit.distance = hitBuffer.block.distance;
    outHit.proxyType = ECollisionProxyType::END;
    outHit.isBackFace = normalizedDirection.Dot(outHit.normal) > 0.f;

    if (hitBuffer.block.shape)
    {
        const PxFilterData filterData = hitBuffer.block.shape->getQueryFilterData();
        if (filterData.word0 < static_cast<PxU32>(ECollisionProxyType::END))
            outHit.proxyType = static_cast<ECollisionProxyType>(filterData.word0);
    }

    return true;
}

void PhysXMgr::Clear_StaticGeometry()
{
    // 레벨 교체 또는 재등록 시 static actor와 cooked triangle mesh를 먼저 해제한다.
    if (!_context)
        return;

    if (_context->scene)
    {
        for (PxRigidStatic* actor : _context->staticActors)
        {
            if (!actor)
                continue;

            _context->scene->removeActor(*actor);
            actor->release();
        }
    }

    _context->staticActors.clear();

    for (PxTriangleMesh* triangleMesh : _context->triangleMeshes)
    {
        if (triangleMesh)
            triangleMesh->release();
    }

    _context->triangleMeshes.clear();
}

bool PhysXMgr::Is_Ready() const
{
    return _context
        && _context->foundation
        && _context->physics
        && _context->scene
        && _context->material;
}

Unique<PhysXMgr> PhysXMgr::Create()
{
    return make_unique<PhysXMgr>();
}

void PhysXMgr::Free()
{
    // PhysX 객체는 생성 역순으로 release해야 dangling reference를 피할 수 있다.
    if (!_context)
        return;

    Clear_StaticGeometry();

    if (_context->material)
    {
        _context->material->release();
        _context->material = nullptr;
    }

    if (_context->scene)
    {
        _context->scene->release();
        _context->scene = nullptr;
    }

    if (_context->dispatcher)
    {
        _context->dispatcher->release();
        _context->dispatcher = nullptr;
    }

    if (_context->physics)
    {
        _context->physics->release();
        _context->physics = nullptr;
    }

    if (_context->foundation)
    {
        _context->foundation->release();
        _context->foundation = nullptr;
    }

    delete _context;
    _context = nullptr;

    Base::Free();
}

NS_END
