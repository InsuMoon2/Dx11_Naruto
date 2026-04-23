#include "pch.h"
#include "Scene_View.h"
#include "Mesh.h"
#include "Action_Command.h"
#include "Camera.h"
#include "GameObject.h"
#include "Event_Manager.h"
#include "RenderTarget.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Input_Manager.h"
#include "Hierarchy.h"
#include "Camera_Free.h"
#include "CollisionProxyActor.h"
#include "Spawn_Helper.h"
#include "StaticMeshActor.h"
#include "Model.h"
#include "Notification_Manager.h"

static bool Try_BuildPickingLocalBounds(Shared<Model> model, BoundingBox& outBounds)
{
    if (!model)
        return false;

    Vec3 minPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasAnyVertex = false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        if (positions.empty())
            continue;

        for (const auto& localPos : positions)
        {
            minPos.x = min(minPos.x, localPos.x);
            minPos.y = min(minPos.y, localPos.y);
            minPos.z = min(minPos.z, localPos.z);

            maxPos.x = max(maxPos.x, localPos.x);
            maxPos.y = max(maxPos.y, localPos.y);
            maxPos.z = max(maxPos.z, localPos.z);

            hasAnyVertex = true;
        }
    }

    if (!hasAnyVertex)
        return false;

    outBounds.Center = (minPos + maxPos) * 0.5f;
    outBounds.Extents = (maxPos - minPos) * 0.5f;

    return true;
}

// 자동 충돌 생성에서 삼각형 단위로 면을 분석하기 위한 로컬 샘플 데이터다.
// Walkable / WallRun 후보를 분리하고, 여러 삼각형을 한 장의 프록시로 묶을 때 사용한다.
struct FCollisionSurfaceSample
{
    array<Vec3, 3> vertices = {};
    Vec3 center = Vec3::Zero;
    Vec3 normal = Vec3::Up;
    float area = 0.f;
};

// 자동 생성된 Walkable 프록시 한 장을 만들기 위해 묶인 상향 면 패치 클러스터다.
// 서로 가까우면서 노멀 방향이 비슷한 삼각형들을 모아, 메시 형상에 더 가까운 경사 Walkable plane 한 장으로 만든다.
struct FWalkableSurfaceCluster
{
    // 패치에 포함된 로컬 정점들이다. 최종 plane 크기와 중심을 투영해서 구할 때 사용한다.
    vector<Vec3> vertices;
    // 클러스터 병합 시 사용하는 로컬 AABB 최소값이다.
    Vec3 minBound = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    // 클러스터 병합 시 사용하는 로컬 AABB 최대값이다.
    Vec3 maxBound = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    // 면적 가중 중심 누적값이다. 최종 plane 중심을 안정적으로 구하기 위해 사용한다.
    Vec3 weightedCenter = Vec3::Zero;
    // 면적 가중 노멀 누적값이다. 최종 plane 회전을 메시 패치 방향에 맞추기 위해 사용한다.
    Vec3 weightedNormal = Vec3::Zero;
    // 누적된 삼각형 총 면적이다.
    float totalArea = 0.f;
    // 클러스터에 들어간 삼각형 개수다.
    uint32 sampleCount = 0;
};

// 벽타기용 WallRun 프록시 한 장을 만들기 위해 묶인 벽 면 패치 클러스터다.
// 서로 가까우면서 노멀 방향이 비슷한 벽 삼각형들을 묶어, 대각선/사면도 포함한 plane 패치로 근사한다.
struct FWallSurfaceCluster
{
    // 런타임에서 어떤 충돌 역할로 분류할지 결정하는 타입이다.
    ECollisionProxyType proxyType = ECollisionProxyType::WallRun;
    // 수평 노멀 방향을 거칠게 양자화한 bin 인덱스다. 곡면/사선 벽을 너무 잘게 쪼개지 않도록 병합 기준으로 사용한다.
    int32 normalBin = -1;
    // 패치에 포함된 로컬 정점들이다. 최종 plane 크기와 중심을 투영해서 구할 때 사용한다.
    vector<Vec3> vertices;
    // 클러스터 병합 시 사용하는 로컬 AABB 최소값이다.
    Vec3 minBound = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    // 클러스터 병합 시 사용하는 로컬 AABB 최대값이다.
    Vec3 maxBound = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    // 면적 가중 중심 누적값이다. 최종 plane 중심을 안정적으로 구하기 위해 사용한다.
    Vec3 weightedCenter = Vec3::Zero;
    // 면적 가중 노멀 누적값이다. 최종 plane 회전을 메시 패치 방향에 맞추기 위해 사용한다.
    Vec3 weightedNormal = Vec3::Zero;
    // 누적된 삼각형 총 면적이다.
    float totalArea = 0.f;
    // 클러스터에 들어간 삼각형 개수다.
    uint32 sampleCount = 0;
};

// 두 구간이 겹치거나 일정 거리 이내로 가까운지 판정한다.
// 삼각형 클러스터를 하나의 프록시로 합칠지 결정할 때 1차 필터로 호출한다.
static bool Intervals_OverlapOrClose(
    float minA,
    float maxA,
    float minB,
    float maxB,
    float tolerance)
{
    if (maxA < minB)
        return (minB - maxA) <= tolerance;

    if (maxB < minA)
        return (minA - maxB) <= tolerance;

    return true;
}

// 클러스터 병합 판단에 사용할 로컬 AABB를 점 하나로 확장한다.
// 삼각형이 추가될 때마다 호출되어 인접 면 여부를 빠르게 판별할 수 있게 한다.
static void Expand_ClusterBounds(Vec3& minBound, Vec3& maxBound, const Vec3& point)
{
    minBound.x = min(minBound.x, point.x);
    minBound.y = min(minBound.y, point.y);
    minBound.z = min(minBound.z, point.z);

    maxBound.x = max(maxBound.x, point.x);
    maxBound.y = max(maxBound.y, point.y);
    maxBound.z = max(maxBound.z, point.z);
}

// 면 노멀을 기준으로 plane 투영에 사용할 접선/종법선 축을 만든다.
// 거의 수직/수평인 면 모두 안정적으로 U/V 축을 만들기 위해 호출한다.
static bool Try_BuildPatchBasis(
    const Vec3& normal,
    Vec3& outTangent,
    Vec3& outBitangent)
{
    Vec3 safeNormal = normal;
    if (safeNormal.LengthSquared() <= FLT_EPSILON)
        return false;

    safeNormal.Normalize();

    const Vec3 upCandidate = fabsf(safeNormal.Dot(Vec3::Up)) > 0.95f
        ? Vec3::Forward
        : Vec3::Up;

    outTangent = upCandidate.Cross(safeNormal);
    if (outTangent.LengthSquared() <= FLT_EPSILON)
        return false;

    outTangent.Normalize();

    outBitangent = safeNormal.Cross(outTangent);
    if (outBitangent.LengthSquared() <= FLT_EPSILON)
        return false;

    outBitangent.Normalize();
    return true;
}

// 벽 노멀의 수평 방향을 일정 개수의 각도 bin으로 양자화한다.
// 곡면 벽이나 사선 벽이 미세한 노멀 차이 때문에 너무 잘게 분리되지 않도록 병합 기준으로 사용한다.
static bool Try_QuantizeWallNormalBin(
    const Vec3& normal,
    int32 binCount,
    int32& outBin)
{
    Vec3 horizontalNormal(normal.x, 0.f, normal.z);
    if (horizontalNormal.LengthSquared() <= FLT_EPSILON)
        return false;

    horizontalNormal.Normalize();

    const float angle = atan2f(horizontalNormal.x, horizontalNormal.z);
    const float wrappedAngle = angle < 0.f ? angle + XM_2PI : angle;
    const float normalized = wrappedAngle / XM_2PI;
    outBin = static_cast<int32>(floorf(normalized * static_cast<float>(binCount))) % binCount;
    return true;
}

// 모델이 보관 중인 CPU 정점/인덱스를 이용해 로컬 삼각형 샘플 목록을 만든다.
// 이후 Walkable / WallRun 자동 생성은 이 삼각형 샘플을 기반으로 면 방향과 범위를 분석한다.
static bool Try_BuildCollisionSurfaceSamples(
    Shared<Model> model,
    vector<FCollisionSurfaceSample>& outSamples)
{
    outSamples.clear();

    if (!model)
        return false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        const auto& indices = mesh->Get_CPUIndices();

        if (positions.empty() || indices.empty())
            continue;

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const uint32 i0 = indices[i];
            const uint32 i1 = indices[i + 1];
            const uint32 i2 = indices[i + 2];

            if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size())
                continue;

            const Vec3 v0 = positions[i0];
            const Vec3 v1 = positions[i1];
            const Vec3 v2 = positions[i2];

            const Vec3 edge0 = v1 - v0;
            const Vec3 edge1 = v2 - v0;
            Vec3 normal = edge0.Cross(edge1);
            const float normalLength = normal.Length();

            if (normalLength <= FLT_EPSILON)
                continue;

            normal /= normalLength;

            FCollisionSurfaceSample sample{};
            sample.vertices = { v0, v1, v2 };
            sample.center = (v0 + v1 + v2) / 3.f;
            sample.normal = normal;
            sample.area = normalLength * 0.5f;

            outSamples.push_back(sample);
        }
    }

    return !outSamples.empty();
}

// Walkable 상향 삼각형 하나를 기존 Walkable 클러스터에 흡수한다.
// 정점/AABB/평균 중심/평균 노멀을 함께 누적해 경사면까지 자연스럽게 plane 패치로 만들 수 있게 한다.
static void Expand_WalkableCluster(
    FWalkableSurfaceCluster& cluster,
    const FCollisionSurfaceSample& sample)
{
    for (const Vec3& vertex : sample.vertices)
    {
        cluster.vertices.push_back(vertex);
        Expand_ClusterBounds(cluster.minBound, cluster.maxBound, vertex);
    }

    cluster.weightedCenter += sample.center * sample.area;
    cluster.weightedNormal += sample.normal * sample.area;
    cluster.totalArea += sample.area;
    ++cluster.sampleCount;
}

// 상향 삼각형 하나가 기존 Walkable 클러스터에 붙을 수 있는지 판단한다.
// 높이 차이와 XZ 인접성을 같이 봐서 떨어진 지붕이나 다른 건물끼리 섞이지 않도록 한다.
static bool Can_MergeWalkableSample(
    const FWalkableSurfaceCluster& cluster,
    const FCollisionSurfaceSample& sample,
    float normalDotThreshold,
    float planeDistanceTolerance,
    float adjacencyTolerance)
{
    Vec3 clusterNormal = cluster.weightedNormal;
    if (clusterNormal.LengthSquared() <= FLT_EPSILON)
        clusterNormal = sample.normal;
    else
        clusterNormal.Normalize();

    if (clusterNormal.Dot(sample.normal) < normalDotThreshold)
        return false;

    const Vec3 clusterCenter = cluster.totalArea > FLT_EPSILON
        ? cluster.weightedCenter / cluster.totalArea
        : sample.center;

    const float planeDistance = fabsf((sample.center - clusterCenter).Dot(clusterNormal));
    if (planeDistance > planeDistanceTolerance)
        return false;

    Vec3 sampleMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 sampleMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const Vec3& vertex : sample.vertices)
    {
        Expand_ClusterBounds(sampleMin, sampleMax, vertex);
    }

    return
        Intervals_OverlapOrClose(cluster.minBound.x, cluster.maxBound.x, sampleMin.x, sampleMax.x, adjacencyTolerance) &&
        Intervals_OverlapOrClose(cluster.minBound.y, cluster.maxBound.y, sampleMin.y, sampleMax.y, adjacencyTolerance) &&
        Intervals_OverlapOrClose(cluster.minBound.z, cluster.maxBound.z, sampleMin.z, sampleMax.z, adjacencyTolerance);
}

// 상향 삼각형들을 지붕/발판 단위로 묶어서 Walkable 클러스터 목록을 만든다.
// 이후 각 클러스터는 하나의 Walkable plane 프록시로 변환된다.
static void Build_WalkableSurfaceClusters(
    const vector<FCollisionSurfaceSample>& samples,
    const BoundingBox& localBounds,
    vector<FWalkableSurfaceCluster>& outClusters)
{
    outClusters.clear();

    const float localWidth = max(localBounds.Extents.x * 2.f, 0.f);
    const float localHeight = max(localBounds.Extents.y * 2.f, 0.f);
    const float localDepth = max(localBounds.Extents.z * 2.f, 0.f);

    const float minUpDot = 0.42f;
    const float normalDotThreshold = 0.94f;
    const float planeDistanceTolerance = max(localHeight * 0.08f, 0.18f);
    const float adjacencyTolerance = max(min(localWidth, localDepth) * 0.05f, 0.22f);

    for (const auto& sample : samples)
    {
        if (sample.normal.y < minUpDot)
            continue;

        bool merged = false;

        for (auto& cluster : outClusters)
        {
            if (!Can_MergeWalkableSample(
                cluster,
                sample,
                normalDotThreshold,
                planeDistanceTolerance,
                adjacencyTolerance))
            {
                continue;
            }

            Expand_WalkableCluster(cluster, sample);
            merged = true;
            break;
        }

        if (!merged)
        {
            FWalkableSurfaceCluster newCluster{};
            Expand_WalkableCluster(newCluster, sample);
            outClusters.push_back(newCluster);
        }
    }
}

// 삼각형 하나가 벽 패치 후보인지 판정한다.
// 완전 바닥/천장 면은 제외하고, 수직 벽부터 기울어진 사면까지 WallRun 패치 후보로 남긴다.
static bool Is_WallCandidateSample(const FCollisionSurfaceSample& sample)
{
    const float minWallSteepness = 0.08f;
    const float maxWallUpDot = 0.72f;
    const float absUpDot = fabsf(sample.normal.y);
    return absUpDot >= minWallSteepness && absUpDot <= maxWallUpDot;
}

// 벽 삼각형 하나를 기존 Wall 클러스터에 흡수한다.
// 정점/AABB/평균 중심/평균 노멀을 함께 누적해 경사면까지 자연스럽게 plane 패치로 만들 수 있게 한다.
static void Expand_WallCluster(
    FWallSurfaceCluster& cluster,
    const FCollisionSurfaceSample& sample)
{
    for (const Vec3& vertex : sample.vertices)
    {
        cluster.vertices.push_back(vertex);
        Expand_ClusterBounds(cluster.minBound, cluster.maxBound, vertex);
    }

    cluster.weightedCenter += sample.center * sample.area;
    cluster.weightedNormal += sample.normal * sample.area;
    cluster.totalArea += sample.area;
    ++cluster.sampleCount;
}

// 벽 삼각형 하나가 기존 Wall 클러스터에 붙을 수 있는지 판단한다.
// 노멀 방향, 같은 평면에 가까운지, 3축 AABB 인접성을 함께 봐서 근처 폴리곤 패치처럼 묶는다.
static bool Can_MergeWallSample(
    const FWallSurfaceCluster& cluster,
    const FCollisionSurfaceSample& sample,
    int32 sampleNormalBin,
    float normalDotThreshold,
    float planeDistanceTolerance,
    float adjacencyTolerance)
{
    if (cluster.normalBin != sampleNormalBin)
        return false;

    Vec3 clusterNormal = cluster.weightedNormal;
    if (clusterNormal.LengthSquared() <= FLT_EPSILON)
        clusterNormal = sample.normal;
    else
        clusterNormal.Normalize();

    if (clusterNormal.Dot(sample.normal) < normalDotThreshold)
        return false;

    const Vec3 clusterCenter = cluster.totalArea > FLT_EPSILON
        ? cluster.weightedCenter / cluster.totalArea
        : sample.center;

    const float planeDistance = fabsf((sample.center - clusterCenter).Dot(clusterNormal));
    if (planeDistance > planeDistanceTolerance)
        return false;

    Vec3 sampleMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 sampleMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const Vec3& vertex : sample.vertices)
    {
        Expand_ClusterBounds(sampleMin, sampleMax, vertex);
    }

    return
        Intervals_OverlapOrClose(cluster.minBound.x, cluster.maxBound.x, sampleMin.x, sampleMax.x, adjacencyTolerance) &&
        Intervals_OverlapOrClose(cluster.minBound.y, cluster.maxBound.y, sampleMin.y, sampleMax.y, adjacencyTolerance) &&
        Intervals_OverlapOrClose(cluster.minBound.z, cluster.maxBound.z, sampleMin.z, sampleMax.z, adjacencyTolerance);
}

// 벽 후보 삼각형들을 근처 + 노멀 유사도 기준으로 묶어서 WallRun 패치 클러스터 목록을 만든다.
// 이후 각 클러스터는 메시 형상에 가까운 기울어진 plane 프록시 한 장으로 변환된다.
static void Build_WallSurfaceClusters(
    const vector<FCollisionSurfaceSample>& samples,
    const BoundingBox& localBounds,
    vector<FWallSurfaceCluster>& outClusters)
{
    outClusters.clear();

    const float localWidth = max(localBounds.Extents.x * 2.f, 0.f);
    const float localHeight = max(localBounds.Extents.y * 2.f, 0.f);
    const float localDepth = max(localBounds.Extents.z * 2.f, 0.f);

    const int32 wallNormalBinCount = 12;
    const float normalDotThreshold = 0.88f;
    const float planeDistanceTolerance = max(max(localWidth, localDepth) * 0.09f, 0.28f);
    const float adjacencyTolerance = max(max(localWidth, max(localHeight, localDepth)) * 0.06f, 0.36f);

    for (const auto& sample : samples)
    {
        if (!Is_WallCandidateSample(sample))
            continue;

        int32 sampleNormalBin = -1;
        if (!Try_QuantizeWallNormalBin(sample.normal, wallNormalBinCount, sampleNormalBin))
            continue;

        bool merged = false;

        for (auto& cluster : outClusters)
        {
            if (!Can_MergeWallSample(
                cluster,
                sample,
                sampleNormalBin,
                normalDotThreshold,
                planeDistanceTolerance,
                adjacencyTolerance))
            {
                continue;
            }

            Expand_WallCluster(cluster, sample);
            merged = true;
            break;
        }

        if (!merged)
        {
            FWallSurfaceCluster newCluster{};
            newCluster.normalBin = sampleNormalBin;
            Expand_WallCluster(newCluster, sample);
            outClusters.push_back(newCluster);
        }
    }
}

// 패치 클러스터 정점과 평균 노멀을 이용해 plane 프록시의 월드 중심/회전/스케일을 만든다.
// 단순 AABB가 아니라 실제 패치 방향으로 투영해서 크기를 계산하므로 대각선 지붕/벽도 더 자연스럽게 근사한다.
static bool Try_BuildPlaneProxyDescFromVertices(
    const vector<Vec3>& localVertices,
    const Vec3& averagedLocalNormal,
    const Matrix& sourceWorldMatrix,
    float planeWidth,
    float planeDepth,
    float normalOffset,
    float minProjectedWidth,
    float minProjectedHeight,
    Vec3& outWorldCenter,
    Vec3& outProxyScale,
    Quat& outWorldRotation)
{
    if (localVertices.empty())
        return false;

    Vec3 worldNormal = Vec3::TransformNormal(averagedLocalNormal, sourceWorldMatrix);
    if (worldNormal.LengthSquared() <= FLT_EPSILON)
        return false;
    worldNormal.Normalize();

    Vec3 tangent = Vec3::Zero;
    Vec3 bitangent = Vec3::Zero;
    if (!Try_BuildPatchBasis(worldNormal, tangent, bitangent))
        return false;

    Vec3 worldCentroid = Vec3::Zero;
    vector<Vec3> worldVertices;
    worldVertices.reserve(localVertices.size());

    for (const Vec3& localVertex : localVertices)
    {
        const Vec3 worldVertex = Vec3::Transform(localVertex, sourceWorldMatrix);
        worldVertices.push_back(worldVertex);
        worldCentroid += worldVertex;
    }

    worldCentroid /= static_cast<float>(worldVertices.size());

    float minU = FLT_MAX;
    float maxU = -FLT_MAX;
    float minV = FLT_MAX;
    float maxV = -FLT_MAX;

    for (const Vec3& worldVertex : worldVertices)
    {
        const Vec3 relative = worldVertex - worldCentroid;
        const float u = relative.Dot(tangent);
        const float v = relative.Dot(bitangent);

        minU = min(minU, u);
        maxU = max(maxU, u);
        minV = min(minV, v);
        maxV = max(maxV, v);
    }

    const float projectedWidth = max(maxU - minU, 0.f);
    const float projectedHeight = max(maxV - minV, 0.f);

    if (projectedWidth < minProjectedWidth || projectedHeight < minProjectedHeight)
        return false;

    const float centerU = (minU + maxU) * 0.5f;
    const float centerV = (minV + maxV) * 0.5f;

    outWorldCenter =
        worldCentroid +
        tangent * centerU +
        bitangent * centerV +
        worldNormal * normalOffset;

    outProxyScale = Vec3(
        projectedWidth / planeWidth,
        1.f,
        projectedHeight / planeDepth);

    outWorldRotation = Quat::FromToRotation(Vec3::Up, worldNormal);
    return true;
}

// Walkable 클러스터를 실제 plane 프록시의 월드 중심/회전/스케일로 변환한다.
// 수평 plane 강제 대신 패치 평균 노멀을 따라가므로 경사진 지붕도 자동 Walkable로 근사할 수 있다.
static bool Try_BuildWalkableProxyDescFromCluster(
    const FWalkableSurfaceCluster& cluster,
    const Matrix& sourceWorldMatrix,
    float planeWidth,
    float planeDepth,
    float roofOffset,
    Vec3& outWorldCenter,
    Vec3& outProxyScale,
    Quat& outWorldRotation)
{
    const float walkableShrinkRatio = 0.92f;
    const Vec3 averagedNormal = cluster.totalArea > FLT_EPSILON
        ? cluster.weightedNormal / cluster.totalArea
        : Vec3::Up;

    if (!Try_BuildPlaneProxyDescFromVertices(
        cluster.vertices,
        averagedNormal,
        sourceWorldMatrix,
        planeWidth,
        planeDepth,
        roofOffset,
        0.5f,
        0.5f,
        outWorldCenter,
        outProxyScale,
        outWorldRotation))
    {
        return false;
    }

    outProxyScale.x *= walkableShrinkRatio;
    outProxyScale.z *= walkableShrinkRatio;
    return true;
}

// Wall 클러스터를 실제 plane 프록시의 월드 중심/회전/스케일로 변환한다.
// 전/후/좌/우 축 고정 대신 패치 평균 노멀을 따라가므로 대각선 벽/사면도 더 자연스럽게 근사할 수 있다.
static bool Try_BuildWallProxyDescFromCluster(
    const FWallSurfaceCluster& cluster,
    const Matrix& sourceWorldMatrix,
    float planeWidth,
    float planeDepth,
    float outwardOffset,
    Vec3& outWorldCenter,
    Vec3& outProxyScale,
    Quat& outWorldRotation)
{
    const float wallShrinkRatio = 0.96f;
    const Vec3 averagedNormal = cluster.totalArea > FLT_EPSILON
        ? cluster.weightedNormal / cluster.totalArea
        : Vec3::Forward;

    if (cluster.totalArea < 3.f)
        return false;

    if (!Try_BuildPlaneProxyDescFromVertices(
        cluster.vertices,
        averagedNormal,
        sourceWorldMatrix,
        planeWidth,
        planeDepth,
        outwardOffset,
        1.2f,
        1.8f,
        outWorldCenter,
        outProxyScale,
        outWorldRotation))
    {
        return false;
    }

    outProxyScale.x *= wallShrinkRatio;
    outProxyScale.z *= wallShrinkRatio;
    return true;
}

// 동일 메쉬에 대해 충돌체 만들기를 다시 눌렀을 때, 예전 넓은 프록시가 남지 않도록 기존 자동 생성 프록시를 먼저 정리한다.
// 이름 규칙이 고정되어 있으므로, baseName + suffix 조합으로 찾아서 삭제 이벤트를 발행한다.
static void Delete_ExistingCollisionProxySet(const wstring& baseName)
{
    static const array<wstring, 5> legacySuffixes = {
        L"_WallFrontProxy",
        L"_WallBackProxy",
        L"_WallLeftProxy",
        L"_WallRightProxy",
        L"_WalkableProxy"
    };

    const auto objects = GAME->Get_GameObjects(GAME->Current_Level());
    for (const auto& obj : objects)
    {
        auto proxyActor = dynamic_pointer_cast<CollisionProxyActor>(obj);
        if (!proxyActor)
            continue;

        const wstring& objectName = proxyActor->Get_Name();
        bool shouldDelete = false;

        for (const auto& suffix : legacySuffixes)
        {
            if (objectName == baseName + suffix)
            {
                shouldDelete = true;
                break;
            }
        }

        if (!shouldDelete)
        {
            const wstring autoPrefix = baseName + L"_Auto";
            shouldDelete = objectName.rfind(autoPrefix, 0) == 0;
        }

        if (shouldDelete)
            EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
    }
}

static wstring Get_CollisionUnitPlanePath()
{
    return fs::absolute(
        L"../../Client/Bin/Resources/StaticMesh/CollisionProxy/Meshes/SM_Collision_UnitPlane.meshbin").wstring();
}

static string Resolve_CollisionUnitPlaneGuid()
{
    const wstring unitPlanePath = Get_CollisionUnitPlanePath();

    if (!fs::exists(unitPlanePath))
    {
        LOG_ERROR("Collision unit plane missing: {}", Utils::ToString(unitPlanePath));
        NOTIFY("Collision Unit Plane Missing");
        return "";
    }

    string guid = GAME->Find_AssetGUID(unitPlanePath);
    if (!guid.empty())
        return guid;

    guid = GAME->Register_Asset(unitPlanePath, "model");
    if (guid.empty())
    {
        LOG_ERROR("Collision unit plane register failed: {}", Utils::ToString(unitPlanePath));
        NOTIFY("Collision Unit Plane Register Failed");
    }

    return guid;
}

static bool Try_BuildProxyPlaneBaseSize(const string& modelGuid, Vec2& outPlaneSize)
{
    const wstring resolvedPath = GAME->Resolve_AssetPath(modelGuid);
    if (resolvedPath.empty())
        return false;

    const Matrix preTransform =
        Matrix::CreateScale(1.f) *
        Matrix::CreateRotationY(XMConvertToRadians(180.f));

    auto model = Model::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        EMeshVertexType::StaticMesh,
        Utils::ToString(resolvedPath),
        preTransform,
        true);

    if (!model)
        return false;

    BoundingBox localBounds{};
    if (!Try_BuildPickingLocalBounds(model, localBounds))
        return false;

    const Vec3 size = Vec3(localBounds.Extents.x * 2.f, localBounds.Extents.y * 2.f, localBounds.Extents.z * 2.f);
    outPlaneSize.x = max(size.x, 0.0001f);
    outPlaneSize.y = max(size.z, 0.0001f);

    return true;
}

static void Notify_CollisionProxyCreateResult(int32 createdCount, int32 skippedCount)
{
    if (createdCount <= 0)
    {
        NOTIFY("Collision Proxy Set Failed");
        return;
    }

    if (skippedCount > 0)
    {
        NOTIFY("Collision Proxy Set Created (Some faces skipped)");
        return;
    }

    NOTIFY("Collision Proxy Set Created");
}

static bool Try_BuildPickingWorldBounds(
    Shared<Model> model,
    const Matrix& worldMatrix,
    BoundingBox& outBounds)
{
    if (!model)
        return false;

    Vec3 minPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasAnyVertex = false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        if (positions.empty())
            continue;

        for (const auto& localPos : positions)
        {
            const Vec3 worldPos = Vec3::Transform(localPos, worldMatrix);

            minPos.x = min(minPos.x, worldPos.x);
            minPos.y = min(minPos.y, worldPos.y);
            minPos.z = min(minPos.z, worldPos.z);

            maxPos.x = max(maxPos.x, worldPos.x);
            maxPos.y = max(maxPos.y, worldPos.y);
            maxPos.z = max(maxPos.z, worldPos.z);

            hasAnyVertex = true;
        }
    }

    if (!hasAnyVertex)
        return false;

    outBounds.Center = (minPos + maxPos) * 0.5f;
    outBounds.Extents = (maxPos - minPos) * 0.5f;

    return true;
}

Scene_View::Scene_View()
    : EditorWindow(TEXT("Scene"))
{
}

Scene_View::~Scene_View()
{
}

void Scene_View::Initialize()
{
    EditorWindow::Initialize();

    // RenderTarget 초기화
    _renderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
    _displayRenderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
}

void Scene_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    Update_CameraLerp(timeDelta);

    //Handle_Guizmo_Shotcut();
}

void Scene_View::OnGui()
{
    Prepare_Window();

    ImGuiWindowFlags flags = Get_WindowFlags();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    string str = Utils::ToString(Get_Name());
    ImGui::Begin(str.c_str(), nullptr, flags);
    {
        Update_WindowState();

        Handle_Guizmo_Shotcut();

        Render_Viewport();

        ImGui::SetCursorPos(ImVec2(12.f, 30.f));
        ImGui::BeginChild("##SceneViewOverlay", ImVec2(170.f, 34.f), false, ImGuiWindowFlags_NoScrollbar);
        ImGui::Checkbox("RT Debug", &_showRenderTargetDebug);
        ImGui::EndChild();

        // Scene View에서만, Edit 모드일 때만 보여주기
        //if (GAME->Get_GameState() == EGameState::Edit)
        {
            Update_ImGuizmo();
        }

    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void Scene_View::Pre_Render()
{
    EditorWindow::Pre_Render();

    Render_Preview();
}

void Scene_View::Focus_OnPosition(const Vec3& targetPos)
{
    _isCameraLerping = true;
    _lerpTargetPos = targetPos;
}

void Scene_View::Update_CameraLerp(float timeDelta)
{
    if (!_isCameraLerping)
        return;

    auto camera = GAME->Get_ActiveCamera();
    if (!camera)
    {
        _isCameraLerping = false; 
        return;
    }

    auto camTransform = camera->Get_Component<Transform>();
    CHECK_NULL(camTransform);

    Vec3 currentPos = camTransform->Get_WorldPosition();

    // 타겟 뒤쪽 계산
    Vec3 look = camTransform->Get_WorldForward();
    Vec3 targetCamPos = _lerpTargetPos - look * _lerpDistance;
    targetCamPos.y += 4.f;

    // Lerp
    Vec3 newPos = Vec3::Lerp(currentPos, targetCamPos, timeDelta * 5.f);
    camTransform->Set_WorldPosition(newPos);

    // 도착하면 종료
    if ((newPos - targetCamPos).Length() < 0.1f)
        _isCameraLerping = false;
}

Ray Scene_View::Build_PickingRay(Vec2 localMousePos) const
{
    float ndcX = (localMousePos.x / _viewportSize.x) * 2.f - 1.f;
    float ndcY = 1.f - (localMousePos.y / _viewportSize.y) * 2.f;

    const Matrix* invertView = GAME->Get_TransformInverse(ETransformState::View);
    const Matrix* invertProj = GAME->Get_TransformInverse(ETransformState::Proj);

    if (!invertView || !invertProj)
    {
        return Ray(Vec3::Zero, Vec3::Forward);
    }

    Vec3 nearNdc(ndcX, ndcY, 0.f);
    Vec3 farNdc(ndcX, ndcY, 1.f);

    Vec3 nearView = Vec3::Transform(nearNdc, *invertProj);
    Vec3 farView = Vec3::Transform(farNdc, *invertProj);

    Vec3 nearWorld = Vec3::Transform(nearView, *invertView);
    Vec3 farWorld = Vec3::Transform(farView, *invertView);

    Vec3 rayDir = farWorld - nearWorld;
    rayDir = Utils::Safe_Normalize(rayDir, Vec3::Forward);

    return Ray(nearWorld, rayDir);
}

bool Scene_View::Try_RaycastScene(const Ray& ray, Vec3& outHitPoint, Shared<GameObject>* outHitObject) const
{
    Shared<GameObject> pickedObject = nullptr;
    float closestDist = FLT_MAX;
    Vec3 closestHitPoint = Vec3::Zero;

    const auto gameObjects = GAME->Get_GameObjects(GAME->Current_Level());

    for (const auto& obj : gameObjects)
    {
        if (!obj)
            continue;

        auto transform = obj->Get_Component<Transform>();
        if (!transform)
            continue;

        Shared<Model> model = nullptr;

        if (auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(obj))
        {
            model = staticMesh->Get_Model();
        }
        else if (auto proxyActor = dynamic_pointer_cast<CollisionProxyActor>(obj))
        {
            model = proxyActor->Get_Model();
        }
        else
        {
            continue;
        }

        if (!model)
            continue;

        BoundingBox worldBounds{};
        if (!Try_BuildPickingWorldBounds(model, transform->Get_WorldMatrix(), worldBounds))
            continue;

        float boundsHitDist = 0.f;
        if (!ray.Intersects(worldBounds, boundsHitDist))
            continue;

        if (boundsHitDist > closestDist)
            continue;

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!model->Raycast(ray, transform->Get_WorldMatrix(), hitDist, hitPoint, hitNormal))
            continue;

        if (hitDist < closestDist)
        {
            closestDist = hitDist;
            closestHitPoint = hitPoint;
            pickedObject = obj;
        }
    }

    if (!pickedObject)
        return false;

    outHitPoint = closestHitPoint;

    if (outHitObject)
        *outHitObject = pickedObject;

    return true;
}

Shared<GameObject> Scene_View::Pick_GameObject(const Ray& ray) const
{
    Shared<GameObject> pickedObject = nullptr;
    Vec3 hitPoint = Vec3::Zero;

    if (!Try_RaycastScene(ray, hitPoint, &pickedObject))
        return nullptr;

    return pickedObject;
}

void Scene_View::Handle_MousePicking()
{
    if (!_isHovered || !_isFocused)
        return;

    if (ImGuizmo::IsUsing())
        return;

    if (_previewObject)
        return;

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    ImVec2 mousePos = ImGui::GetMousePos();

    if (mousePos.x < _viewportTopLeft.x || mousePos.x > _viewportBottomRight.x ||
        mousePos.y < _viewportTopLeft.y || mousePos.y > _viewportBottomRight.y)
    {
        return;
    }

    Vec2 localMousePos(
        mousePos.x - _viewportTopLeft.x,
        mousePos.y - _viewportTopLeft.y
    );

    Ray pickingRay = Build_PickingRay(localMousePos);
    auto pickedObject = Pick_GameObject(pickingRay);
    if (!pickedObject)
        return;

    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    if (!hierarchy)
        return;

    const bool isMultiSelect = ImGui::GetIO().KeyCtrl;
    hierarchy->Select_Object(pickedObject, isMultiSelect);
}

void Scene_View::Clear_Drag()
{
    _previewObject = nullptr;
    _isDraggingPrefab = false;
}

Shared<GameObject> Scene_View::Create_StaticMesh(const string& guid, const Vec3& position)
{
    wstring assetPath = GAME->Resolve_AssetPath(guid);
    if (assetPath.empty())
    {
        LOG_ERROR("Create_StaticMeshActor: GUID {} 를 찾을 수 없습니다.", guid);
        return nullptr;
    }

    StaticMeshActor::FStaticMeshDesc desc;
    desc.modelGuid = guid;
    desc.name = Utils::ToWString(fs::path(assetPath).stem().string());

    auto meshActor = StaticMeshActor::Create(GAME->Get_Device(), GAME->Get_Context());
    if (!meshActor)
        return nullptr;

    if (FAILED(meshActor->Initialize(&desc)))
        return nullptr;

    auto transform = meshActor->Get_Component<Transform>();
    if (transform)
        transform->Set_WorldPosition(position);

    return meshActor;
}

Shared<GameObject> Scene_View::Create_CollisionProxy(
    const CollisionProxyActor::FCollisionProxyDesc& desc)
{
    auto proxyActor = CollisionProxyActor::Create(GAME->Get_Device(), GAME->Get_Context());
    if (!proxyActor)
        return nullptr;

    auto proxyDesc = desc;

    if (FAILED(proxyActor->Initialize(&proxyDesc)))
        return nullptr;

    return proxyActor;
}

void Scene_View::Create_CollisionProxySetFromStaticMesh(Shared<GameObject> sourceObj)
{
    auto staticMesh = dynamic_pointer_cast<StaticMeshActor>(sourceObj);
    CHECK_NULL(staticMesh);

    auto sourceTransform = staticMesh->Get_Component<Transform>();
    if (!sourceTransform)
        return;

    auto sourceModel = staticMesh->Get_Model();
    if (!sourceModel)
        return;

    const string unitPlaneGuid = Resolve_CollisionUnitPlaneGuid();
    if (unitPlaneGuid.empty())
        return;

    Vec2 planeBaseSize(1.f, 1.f);
    if (!Try_BuildProxyPlaneBaseSize(unitPlaneGuid, planeBaseSize))
    {
        LOG_ERROR("Collision unit plane base size build failed");
        NOTIFY("Collision Unit Plane Invalid");
        return;
    }

    BoundingBox localBounds{};
    if (!Try_BuildPickingLocalBounds(sourceModel, localBounds))
    {
        LOG_ERROR(
            "Create_CollisionProxySetFromStaticMesh: local bounds build failed for '{}'",
            Utils::ToString(staticMesh->Get_Name()));
        NOTIFY("Collision Proxy Bounds Failed");
        return;
    }

    const float outwardOffset = 0.05f;
    const float roofOffset = 0.03f;

    const Matrix sourceWorldMatrix = sourceTransform->Get_WorldMatrix();
    const wstring baseName = staticMesh->Get_Name();

    vector<FCollisionSurfaceSample> surfaceSamples;
    if (!Try_BuildCollisionSurfaceSamples(sourceModel, surfaceSamples))
    {
        LOG_ERROR(
            "Create_CollisionProxySetFromStaticMesh: surface sample build failed for '{}'",
            Utils::ToString(staticMesh->Get_Name()));
        NOTIFY("Collision Proxy Surface Failed");
        return;
    }

    vector<FWalkableSurfaceCluster> walkableClusters;
    Build_WalkableSurfaceClusters(surfaceSamples, localBounds, walkableClusters);

    vector<FWallSurfaceCluster> wallClusters;
    Build_WallSurfaceClusters(surfaceSamples, localBounds, wallClusters);

    Delete_ExistingCollisionProxySet(baseName);

    int32 createdCount = 0;
    int32 skippedCount = 0;

    auto createPlaneProxy =
        [&](const wstring& proxyName,
            ECollisionProxyType proxyType,
            const Vec3& worldCenter,
            const Quat& worldRotation,
            const Vec3& proxyScale)
        {
            CollisionProxyActor::FCollisionProxyDesc desc{};
            desc.name = proxyName;
            desc.modelGuid = unitPlaneGuid;
            desc.proxyType = proxyType;

            auto proxyObj = Create_CollisionProxy(desc);
            if (!proxyObj)
            {
                ++skippedCount;
                return;
            }

            auto proxyTransform = proxyObj->Get_Component<Transform>();
            if (!proxyTransform)
            {
                ++skippedCount;
                return;
            }

            proxyTransform->Set_WorldPosition(worldCenter);
            proxyTransform->Set_WorldRotation(worldRotation);
            proxyTransform->Set_LocalScale(proxyScale);

            GAME->Add_GameObject(GAME->Current_Level(), TEXT("Layer_CollisionProxy"), proxyObj);
            ++createdCount;
        };

    int32 walkableProxyIndex = 0;
    for (const auto& walkableCluster : walkableClusters)
    {
        Vec3 worldCenter = Vec3::Zero;
        Vec3 proxyScale(1.f, 1.f, 1.f);
        Quat worldRotation = Quat::Identity;

        if (!Try_BuildWalkableProxyDescFromCluster(
            walkableCluster,
            sourceWorldMatrix,
            planeBaseSize.x,
            planeBaseSize.y,
            roofOffset,
            worldCenter,
            proxyScale,
            worldRotation))
        {
            ++skippedCount;
            continue;
        }

        createPlaneProxy(
            baseName + L"_AutoWalkableProxy_" + std::to_wstring(walkableProxyIndex++),
            ECollisionProxyType::Walkable,
            worldCenter,
            worldRotation,
            proxyScale);
    }

    int32 wallProxyIndex = 0;
    for (const auto& wallCluster : wallClusters)
    {
        Vec3 worldCenter = Vec3::Zero;
        Vec3 proxyScale(1.f, 1.f, 1.f);
        Quat worldRotation = Quat::Identity;

        if (!Try_BuildWallProxyDescFromCluster(
            wallCluster,
            sourceWorldMatrix,
            planeBaseSize.x,
            planeBaseSize.y,
            outwardOffset,
            worldCenter,
            proxyScale,
            worldRotation))
        {
            ++skippedCount;
            continue;
        }

        createPlaneProxy(
            baseName + L"_AutoWallProxy_" + std::to_wstring(wallProxyIndex++),
            ECollisionProxyType::WallRun,
            worldCenter,
            worldRotation,
            proxyScale);
    }

    LOG_INFO(
        "Collision proxy set created: actor='{}', walkableClusters={}, wallClusters={}, created={}, skipped={}",
        Utils::ToString(baseName),
        static_cast<int32>(walkableClusters.size()),
        static_cast<int32>(wallClusters.size()),
        createdCount,
        skippedCount);

    Notify_CollisionProxyCreateResult(createdCount, skippedCount);
}

void Scene_View::Handle_DragDrop(Shared<GameObject> previewObj, const Vec3& worldPos, const ImGuiPayload* payload)
{
    if (!_previewObject)
    {
        _previewObject = previewObj;
        _isDraggingPrefab = true;
    }

    if (_previewObject)
    {
        auto transform = _previewObject->Get_Component<Transform>();
        if (transform) transform->Set_WorldPosition(worldPos);
    }

    if (payload->IsDelivery() && _previewObject)
    {
        GAME->Add_GameObject(GAME->Current_Level(),
            TEXT("Layer_GameObject"), _previewObject);

        auto hierarchy = dynamic_pointer_cast<Hierarchy>(
            EDITOR->Get_Window(TEXT("Hierarchy")));
        if (hierarchy) hierarchy->Select_Object(_previewObject, false);

        Clear_Drag();
    }

}

ImGuiWindowFlags Scene_View::Get_WindowFlags() const
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if (_isFullScreen)
    {
        flags |= ImGuiWindowFlags_NoDecoration;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoResize;
    }

    return flags;
}

void Scene_View::Prepare_Window()
{
    // DockID 복원
    if (_shouldRestoreWindow && _savedDockId != 0)
    {
        ImGui::SetNextWindowDockID(_savedDockId, ImGuiCond_Always);
        _shouldRestoreWindow = false;
    }

    // 전체화면 크기 설정
    if (_isFullScreen)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    }
}

void Scene_View::Render_Viewport()
{
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    _viewportSize = Vec2(panelSize.x, panelSize.y);

    if (_renderTarget && panelSize.x > 0 && panelSize.y > 0)
    {
        _renderTarget->Resize(static_cast<uint32>(panelSize.x),
            static_cast<uint32>(panelSize.y));

        if (_displayRenderTarget)
        {
            _displayRenderTarget->Resize(static_cast<uint32>(panelSize.x),
                static_cast<uint32>(panelSize.y));
        }

        auto srv = _displayRenderTarget ? _displayRenderTarget->Get_SRV() : _renderTarget->Get_SRV();

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)srv, panelSize);

        _viewportTopLeft = imageTopLeft;
        _viewportBottomRight = ImVec2(imageTopLeft.x + panelSize.x, imageTopLeft.y + panelSize.y);

        if (ImGui::BeginDragDropTarget())
        {
            ImVec2 mousePos = ImGui::GetMousePos();

            Vec2 localPos(
                mousePos.x - _viewportTopLeft.x,
                mousePos.y - _viewportTopLeft.y
            );

            Vec3 worldPos = Screen_To_World(localPos);

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                "CONTENT_PREFAB",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                string guid = (const char*)payload->Data;
                string prefabName = GUID_To_PrefabName(guid);

                auto obj = _previewObject ? nullptr : GAME->Instantiate_Prefab(prefabName);
                Handle_DragDrop(obj, worldPos, payload);
            }

            if (auto* payload = ImGui::AcceptDragDropPayload(
                "CONTENT_MESH",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                string guid = (const char*)payload->Data;
                auto obj = _previewObject ? nullptr : Create_StaticMesh(guid, worldPos);

                Handle_DragDrop(obj, worldPos, payload);
            }

            ImGui::EndDragDropTarget();
        }
        else
        {
            if (_previewObject)
            {
                Clear_Drag();
            }
        }

        Handle_MousePicking();
    }
}

void Scene_View::Update_WindowState()
{
    if (!_isFullScreen)
        _savedDockId = ImGui::GetWindowDockID();

    _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    _isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    const bool canControlSceneCamera = _isFocused && _isHovered;

    auto freeCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_FREE);
    if (freeCam)
    {
        auto camFree = dynamic_pointer_cast<Camera_Free>(freeCam);
        if (camFree)
            camFree->Set_InputEnabled(canControlSceneCamera);

    }
}

void Scene_View::ToggleFullScreen()
{
    _isFullScreen = !_isFullScreen;

    HWND hWnd = EDITOR->Get_WindowHandle();

    if (_isFullScreen)
    {
        // 현재 윈도우 크기/스타일 저장
        GetWindowRect(hWnd, &_windowedRect);
        _savedStyle = GetWindowLongPtr(hWnd, GWL_STYLE);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight,
            SWP_FRAMECHANGED);

        GAME->Resize_BackBuffer(screenWidth, screenHeight);
    }
    else
    {
        // 원래 윈도우 스타일/크기 복원
        SetWindowLongPtr(hWnd, GWL_STYLE, _savedStyle);
        SetWindowPos(hWnd, HWND_TOP,
            _windowedRect.left, _windowedRect.top,
            _windowedRect.right - _windowedRect.left,
            _windowedRect.bottom - _windowedRect.top,
            SWP_FRAMECHANGED);

        // Graphics 리사이즈
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        GAME->Resize_BackBuffer(clientRect.right, clientRect.bottom);

        _shouldRestoreWindow = true; 
    }
}

void Scene_View::Update_ImGuizmo()
{
    if (_gizmoOperation == (ImGuizmo::OPERATION)0)
        return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 wPos = ImGui::GetWindowPos();

    float x = vMin.x + wPos.x;
    float y = vMin.y + wPos.y;

    ImGuizmo::SetRect(x, y, _viewportSize.x, _viewportSize.y);

    // 선택된 오브젝트
    auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    //if (hierarchy)
    //{
    //    bool isMultiSelect = ImGui::GetIO().KeyCtrl;
    //    hierarchy->Select_Object(pickedObject, isMultiSelect);
    //}
    CHECK_NULL(hierarchy);

    const auto& selectedObjects = hierarchy->Get_SelectedObject();
    if (selectedObjects.empty())
        return;

    // 이건 선택
    auto targetObject = selectedObjects[0]; // 첫 번째 객체만 조작
    CHECK_NULL(targetObject);

    auto transform = targetObject->Get_Component<Transform>();
    CHECK_NULL(transform);

    // 행렬
    const Matrix* pView = GAME->Get_Transform(ETransformState::View);
    const Matrix* pProj = GAME->Get_Transform(ETransformState::Proj);
    if (!pView || !pProj) return;

    Matrix world = transform->Get_WorldMatrix();
    Matrix view = *pView;
    Matrix proj = *pProj;

    Matrix worldForGizmo = world;

    ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0],
        _gizmoOperation, _gizmoMode, &worldForGizmo.m[0][0]);

    if (ImGuizmo::IsUsing())
    {
        if (!_gizmoWasUsing)
        {
            _gizmoStartPos   = transform->Get_LocalPosition();
            _gizmoStartRot   = transform->Get_LocalRotation();
            _gizmoStartScale = transform->Get_LocalScale();
            _gizmoWasUsing = true;
        }

        if (ImGui::GetIO().KeyAlt && !_altDragDuplicated)
        {
            EVENT->Publish(FEvent_Object::Create(EEventType::Create_Object, targetObject));
            _altDragDuplicated = true;
        }

        Vec3 scale, translation;
        Quat rotation;

        worldForGizmo.Decompose(scale, rotation, translation);

        transform->Set_WorldPosition(translation);
        transform->Set_WorldRotation(rotation);

        // 스케일 (부모가 있을 경우 보정 필요)
        if (transform->Get_Parent())
        {
            Vec3 parentScale = transform->Get_Parent()->Get_WorldScale();
            // 0 나누기 방지
            if (parentScale.LengthSquared() > 0.0001f)
            {
                transform->Set_LocalScale(scale / parentScale);
            }
        }

        else
        {
            transform->Set_LocalScale(scale);
        }
    }
    else
    {
        _altDragDuplicated = false;

        // 기즈모 조작 끝
        if (_gizmoWasUsing)
        {
            Vec3 newPos = transform->Get_LocalPosition();
            Quat newRot = transform->Get_LocalRotation();
            Vec3 newScale = transform->Get_LocalScale();

            auto cmd = Action_Command::Create(
                [=]()
                { // Undo
                    transform->Set_LocalPosition(_gizmoStartPos);
                    transform->Set_LocalRotation(_gizmoStartRot);
                    transform->Set_LocalScale(_gizmoStartScale);
                },
                [=]()// Redo
                {
                    transform->Set_LocalPosition(newPos);
                    transform->Set_LocalRotation(newRot);
                    transform->Set_LocalScale(newScale);
                },
                "Transform Gizmo Edit");

            EDITOR->ExecuteCommand(cmd);

            _gizmoWasUsing = false;
        }

    }

}

void Scene_View::Handle_Guizmo_Shotcut()
{
    if (!ImGuizmo::IsUsing())
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            return;

        if (ImGui::IsKeyPressed(ImGuiKey_Q))
            _gizmoOperation = (ImGuizmo::OPERATION)0;

        if (ImGui::IsKeyPressed(ImGuiKey_W))
            _gizmoOperation = ImGuizmo::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_E))
            _gizmoOperation = ImGuizmo::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_R))
            _gizmoOperation = ImGuizmo::SCALE;
    }
    
}

Vec3 Scene_View::Screen_To_World(Vec2 screenPos)
{
    const Ray pickingRay = Build_PickingRay(screenPos);
    Vec3 hitPoint = Vec3::Zero;

    if (Try_RaycastScene(pickingRay, hitPoint))
        return hitPoint;

    const Vec3 nearWorld = pickingRay.position;
    const Vec3 rayDir = pickingRay.direction;

    float targetY = 0.f;
    if (fabsf(rayDir.y) < FLT_EPSILON)
        return Vec3(nearWorld.x, targetY, nearWorld.z);

    float t = (targetY - nearWorld.y) / rayDir.y;

    return nearWorld + rayDir * t;
}

string Scene_View::GUID_To_PrefabName(const string& guid)
{
    wstring assetPath = GAME->Resolve_AssetPath(guid);
    if (assetPath.empty())
    {
        LOG_ERROR("Unknown asset GUID: {}", guid);
        return "";
    }

    fs::path path(assetPath);
    string fileName = path.filename().string();
    size_t dotPos = fileName.find('.');

    return (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();
}

void Scene_View::Spawn_Prefab(const string& guid, const Vec3& worldPos)
{
#pragma region Legacy : 경로 기반 소환
    // 파일 경로에서 프리펩 이름 추출
    //fs::path path(prefabPath);
    //string fileName = path.filename().string();
    //size_t dotPos = fileName.find('.');
    //string prefabName = (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();
    //
    //auto newObj = GAME->Instantiate_Prefab(prefabName);
    //
    //if (!newObj)
    //{
    //    LOG_ERROR("Failed to instantiate prefab: {}", prefabName);
    //    return;
    //}
    //
    //// UUID 출력 로그 확인
    //LOG_INFO("Spawned '{}' UUID: {}",
    //    Utils::ToString(newObj->Get_Name()),
    //    newObj->Get_GUID());
    //
    //// 위치 세팅
    //auto transform = newObj->Get_Component<Transform>();
    //if (transform)
    //{
    //    transform->Set_WorldPosition(worldPos);
    //}
    //
    //// 스폰
    //GAME->Add_GameObject(ETOI(ELevelType::GamePlay), TEXT("Layer_GamePlay"), newObj);
    //
    //LOG_INFO("Position: ({:.2f}, {:.2f}, {:.2f})", worldPos.x, worldPos.y, worldPos.z);
    //
    //auto hierarchy = dynamic_pointer_cast<Hierarchy>(EDITOR->Get_Window(TEXT("Hierarchy")));
    //if (hierarchy)
    //{
    //    hierarchy->Select_Object(newObj, false);  // false = 단일 선택
    //}
#pragma endregion

    string prefabName = GUID_To_PrefabName(guid);
    if (prefabName.empty()) return;

    auto newObj = Spawn_Helper::Prefab(prefabName)
        .AtLevel(GAME->Current_Level())
        .InLayer(TEXT("Layer_GameObject"))
        .Position(worldPos)
        .Spawn();

    if (!newObj)
        return;

    auto hierarchy = dynamic_pointer_cast<Hierarchy>(
        EDITOR->Get_Window(TEXT("Hierarchy")));

    if (hierarchy)
        hierarchy->Select_Object(newObj, false);

}

void Scene_View::Render_Preview()
{
    if (!_previewObject)
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, _previewObject);
}

shared_ptr<Scene_View> Scene_View::Create()
{
    return make_shared<Scene_View>();
}
