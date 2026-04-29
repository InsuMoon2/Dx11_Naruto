#pragma once

#include "Component.h"

NS_BEGIN(Client)

class EnemyCharacter;

class TargetComponent : public Component
{
    GENERATED_COMPONENT(TargetComponent, Protocol::COMPONENT_TYPE_TARGET)

public:
    explicit TargetComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit TargetComponent(const TargetComponent& rhs);
    virtual ~TargetComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Late_Update(float timeDelta);

    void    Update_Targeting(float timeDelta);

public:
    const vector<Weak<Character>>& Get_Candiates() const { return _candidates; }
    Weak<Character>                Get_LockedTarget() const { return _lockedTarget; }

    bool                        IsLockOn() const { return _isLocked; }
    void                        Set_TargetCollider(Shared<Collider> targetCollider) { _targetCollider = targetCollider; }

private:
    void                        Update_Candiates();

    void                        Refresh_LockedTarget();
    // 현재 락온 중이어도 후보들 중 가장 가까운 적으로 자동 전환할 때 호출한다.
    void                        Auto_SwitchToNearestTarget();
    void                        Clear_Lock();
    void                        LockOn_NearestTarget(bool currentTarget = false);
    Shared<Character>           Find_NearestTarget(bool currentTarget = false);
    bool                        Is_TargetInCandidates(Shared<GameObject> target);

private:
    Shared<Collider>            _targetCollider;
    Weak<Character>             _lockedTarget; // V 키로 타겟팅할 놈

    bool                        _isLocked = false;
    vector<Weak<Character>>     _candidates;   // 타겟팅된 몬스터 목록들
    

public:
    static Shared<TargetComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
