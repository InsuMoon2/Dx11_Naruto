#pragma once

#include "EnemyCharacter.h"

NS_BEGIN(Client)

class Boss_Pain final : public EnemyCharacter
{
    GENERATED_BODY(Boss_Pain)

public:
    explicit Boss_Pain(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Boss_Pain(const Boss_Pain& rhs);
    virtual ~Boss_Pain() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    json    To_Json() const override;
    void    From_Json(const json& data) override;

protected:
    HRESULT Ready_Components() override;

    Protocol::OBJECT_TYPE Get_EnemyObjectType() const override;
    Protocol::OBJECT_STATE_TYPE To_EnemyObjectState(const string& animStateName) const override;

private:
    float _maxHp = 1500.f; 
    float _attack = 35.f; 
    float _maxWalkSpeed = 2.f;
    float _maxSprintSpeed = 4.f;

    Vec3 _bodyColliderCenter = Vec3(0.f, 1.5f, 0.f);
    Vec3 _bodyColliderExtents = Vec3(1.f, 1.5f, 1.f);

    uint32 _modelComponentID = static_cast<uint32>(std::hash<string>{}("Model_Pain")); 

public:
    static Shared<Boss_Pain> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
