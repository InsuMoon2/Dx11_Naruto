#pragma once

#include "Base.h"
#include "MovementComponent.h"

NS_BEGIN(Engine)

struct FProxyEntry
{
    MovementComponent::FCollisionModelInstance instance;

    uint8 proxyType = 0;
};

class ENGINE_DLL CollisionProxy_Manager : public Base
{
public:
    explicit CollisionProxy_Manager() = default;
    virtual ~CollisionProxy_Manager() = default;

public:
    void Ready_CollisionProxy(const vector<FProxyEntry>& entries);

    void Query_ActiveCollisionProxy(
        const Vec3& focusPos,
        vector<MovementComponent::FCollisionModelInstance>& outWalkable,
        vector<MovementComponent::FCollisionModelInstance>& outWall) const;

    void Clear();

private:
    static bool Is_EntryInActiveRange(const FProxyEntry& entry, const Vec3& focusPos, float activeRangeSq);

private:
    vector<FProxyEntry> _entries;

    float _activeRange = 40.f;

public:
    static Unique<CollisionProxy_Manager> Create();
    void Free() override;
};

NS_END
