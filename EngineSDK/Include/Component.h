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
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* arg);

    virtual uint32 Get_ComponentID() const = 0;

    Shared<GameObject> Get_Owner() { return _owner.lock(); }
    void    Set_Owner(Shared<GameObject> owner) { _owner = owner; }

public:
    virtual json    To_Json() const;
    virtual void    From_Json(const json& data);

protected:
    Shared<Component> GetSharedPtr() { return static_pointer_cast<Component>(shared_from_this()); }

protected:
    ComPtr<Device> _device = { nullptr };
    ComPtr<DeviceContext> _context = { nullptr };

protected:
    Weak<GameObject> _owner;

public:
    virtual Shared<Component> Clone(void* arg) abstract;
    virtual void Free() override;
};

NS_END
