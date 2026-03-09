#pragma once

#include "Character.h"

NS_BEGIN(Engine)
class BehaviorTree;
class Model;
NS_END

NS_BEGIN(Client)

class CombatStat;
class MovementComponent;
class AIController;

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

public:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

protected:
    HRESULT Ready_Components() override;
    HRESULT Bind_ShaderResources() override;

private:
    Shared<Model>               _model{};

    Shared<CombatStat>          _combatStat{};
    Shared<MovementComponent>   _movement{};
    Shared<AIController>        _aiController{};
    Shared<BehaviorTree>        _behavior{};

    float _test = 10.f;

public:
    static Shared<Monster> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
