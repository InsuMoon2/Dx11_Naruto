#pragma once

#include "Monster.h"

NS_BEGIN(Client)

class CharkraMove_Component;

class Monster_Leaf : public Monster
{
    GENERATED_BODY(Monster_Leaf)

public:
    explicit Monster_Leaf(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Monster_Leaf(const Monster_Leaf& rhs);
    virtual ~Monster_Leaf() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    BeginPlay() override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Ready_Components() override;
    Protocol::OBJECT_TYPE Get_EnemyObjectType() const override;

private:
    bool _glovePartsReady = false;

    HRESULT Ready_GloveParts();

private:
    Shared<CharkraMove_Component> _chakraTrail;

public:
    static Shared<Monster_Leaf> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
