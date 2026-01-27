#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL GameObject abstract : public Base
{
public:
    explicit GameObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit GameObject(const GameObject& rhs);
    virtual ~GameObject();

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* arg);
    virtual void    Priority_Update(float timeDelta);
    virtual void    Update(float timeDelta);
    virtual void    Late_Update(float timeDelta);

protected:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

public:
    virtual shared_ptr<GameObject> Clone(void* arg) abstract;
    virtual void Free() override;


};

NS_END
