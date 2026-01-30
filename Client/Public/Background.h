#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class Background : public GameObject
{
public:
    explicit Background(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Background(const Background& rhs);
    virtual ~Background();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(any arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

protected:


public:
    static shared_ptr<GameObject>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<GameObject>          Clone(any arg) override;
    virtual void                    Free() override;
};

NS_END
