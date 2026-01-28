#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class ENGINE_DLL Component abstract : public Base
{
public:
    explicit Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Component(const Component& rhs);
    virtual ~Component();

public:
    HRESULT Initialize();
    HRESULT Initialize_Prototype();

    shared_ptr<GameObject> Get_Owner() { return _owner.lock(); }

protected:
    ComPtr<Device>          _device  = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

protected:
    weak_ptr<GameObject>    _owner;

public:
    virtual shared_ptr<Component> Clone(void* arg) abstract;
    virtual void Free() override;

};

NS_END
