#pragma once

#include "Monster.h"

NS_BEGIN(Client)
class CharkraMove_Component;

class Monster_Wood final : public Monster
{
    GENERATED_BODY(Monster_Wood)

public:
    explicit Monster_Wood(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Monster_Wood(const Monster_Wood& rhs);
    virtual ~Monster_Wood() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void Update(float timeDelta) override;
    void Late_Update(float timeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Ready_Components() override;
    Protocol::OBJECT_TYPE Get_EnemyObjectType() const override;

private:
    Shared<CharkraMove_Component> _chakraTrail;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
