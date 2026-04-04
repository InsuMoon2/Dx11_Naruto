#pragma once

#include "Component.h"

NS_BEGIN(Client)

class Monster;

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
    const vector<Weak<Monster>>& Get_Candiates() const { return _candidates; }
    Weak<Monster>                Get_LockedTarget() const { return _lockedTarget; }

    bool                         IsLockOn() const { return _isLocked; }
    void                         Set_TargetCollider(Shared<Collider> targetCollider) { _targetCollider = targetCollider; }

private:
    void                         LockOn_NearestTarget();
    void                         Update_Candiates();

private:
    Shared<Collider>         _targetCollider;
    Weak<Monster>            _lockedTarget; // V 키로 타겟팅할 놈

    bool                     _isLocked = false;
    vector<Weak<Monster>>    _candidates;   // 타겟팅된 몬스터 목록들
    

public:
    static Shared<TargetComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
