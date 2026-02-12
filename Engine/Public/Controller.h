#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Controller abstract : public GameObject
{
    GENERATED_BODY(Controller)

public:
    explicit Controller(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Controller(const Controller& rhs);
    virtual ~Controller();

protected:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;

    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

public:
    void    Free() override;
};

NS_END
