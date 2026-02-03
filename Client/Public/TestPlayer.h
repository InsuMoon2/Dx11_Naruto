#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class TestPlayer : public GameObject
{
    GENERATED_BODY(TestPlayer)

public:
    explicit TestPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit TestPlayer(const TestPlayer& rhs);
    virtual ~TestPlayer() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;

public:
    static shared_ptr<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual shared_ptr<GameObject> Clone(void* arg) override;

};

NS_END
