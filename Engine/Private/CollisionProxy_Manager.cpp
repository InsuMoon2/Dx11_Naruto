#include "pch.h"
#include "CollisionProxy_Manager.h"

void CollisionProxy_Manager::Ready_CollisionProxy(const vector<FProxyEntry>& entries)
{
    _entries = entries;
}

void CollisionProxy_Manager::Query_ActiveCollisionProxy(const Vec3& focusPos,
    vector<MovementComponent::FCollisionModelInstance>& outWalkable,
    vector<MovementComponent::FCollisionModelInstance>& outWall) const
{
    outWalkable.clear();
    outWall.clear();

    const float activeRangeSq = _activeRange * _activeRange;

    for (const auto& entry : _entries)
    {
        if (!Is_EntryInActiveRange(entry, focusPos, activeRangeSq))
            continue;

        const auto proxyType = static_cast<ECollisionProxyType>(entry.proxyType);

        switch (proxyType)
        {
        case ECollisionProxyType::Walkable:
            outWalkable.push_back(entry.instance);
            break;

        case ECollisionProxyType::WallRun:
            outWall.push_back(entry.instance);
            break;

        case ECollisionProxyType::WorldBlock:
            // 1차에서는 movement가 world block proxy를 직접 소비하지 않는다.
            break;

        default:
            break;
        }
    }
}

void CollisionProxy_Manager::Clear()
{
    _entries.clear();
}

bool CollisionProxy_Manager::Is_EntryInActiveRange(const FProxyEntry& entry, const Vec3& focusPos, float activeRangeSq)
{
    const auto& instance = entry.instance;

    if (!instance.hasWorldBounds)
        return false;

    const Vec3 toCenter = instance.worldBounds.Center - focusPos;

    return toCenter.LengthSquared() <= activeRangeSq;
}

Unique<CollisionProxy_Manager> CollisionProxy_Manager::Create()
{
    return make_unique<CollisionProxy_Manager>();
}

void CollisionProxy_Manager::Free()
{
    Base::Free();
}
