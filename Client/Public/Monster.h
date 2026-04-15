#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class BehaviorTree;
class Model;
class Collider;
class MovementComponent;
NS_END

NS_BEGIN(Client)

class CombatStat;
class AIController;
class AnimationStateComponent;

class Monster : public Character
{
    GENERATED_BODY(Monster)

public:
    explicit Monster(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Monster(const Monster& rhs);
    virtual ~Monster();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    HRESULT Bind_Lights() override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    TakeDamage(const FDamageEvent& damageEvent) override;

    void    OnDamaged(const FDamageEvent& damageEvent) override;
    void    OnDead(const FDamageEvent& damageEvent) override;

public:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

public:
    void Set_NetworkDriven(bool enabled) { _networkDriven = enabled; }

    // 서버 ObjectInfo를 현재 몬스터 transform에 반영
    void Sync(const Protocol::ObjectInfo& info);

protected:
    HRESULT Ready_Components() override;
    HRESULT Bind_ShaderResources() override;

private:
    Shared<Model>                   _model;

    Shared<CombatStat>              _combatStat;
    Shared<MovementComponent>       _movement;
    Shared<AIController>            _aiController;
    Shared<BehaviorTree>            _behavior;
    Shared<AnimationStateComponent> _animState;

    Shared<Collider> _collider;

    // AI/BT를 끄고 서버 상태 따르기
    bool _networkDriven = false;

    float _test = 10.f;

public:
    static Shared<Monster> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
