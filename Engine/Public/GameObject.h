#pragma once

#include "Transform.h"
#include "Transform.h"

NS_BEGIN(Engine)

class ENGINE_DLL GameObject abstract : public Base
{
public:
    struct FGameObjectDesc : public Transform::FTransformDesc
    {
        uint32		iFlag = {};
    };

public:
    explicit GameObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit GameObject(const GameObject& rhs);
    virtual ~GameObject();

public:
    virtual HRESULT     Initialize_Prototype();
    virtual HRESULT     Initialize(any arg);
    virtual void        Priority_Update(float timeDelta);
    virtual void        Update(float timeDelta);
    virtual void        Late_Update(float timeDelta);
    virtual void        Render();

public:
    void Add_Component(const wstring& tag, shared_ptr<Component> component);
    shared_ptr<Component> Get_Compoennt(const wstring& tag);

    template<typename T>
    shared_ptr<T> Get_Component(const wstring& tag)
    {
        return static_pointer_cast<T>(Get_Component(tag));
    }

protected:
    shared_ptr<GameObject> GetSharedPtr()
    {
        return static_pointer_cast<GameObject>(shared_from_this());
    }

protected:
    ComPtr<Device> _device = { nullptr };
    ComPtr<DeviceContext> _context = { nullptr };

protected:
    map<wstring, shared_ptr<Component>> _components;

    shared_ptr<Transform> _transformCom;


public:
    virtual shared_ptr<GameObject> Clone(any arg) abstract;
    virtual void Free() override;
};

NS_END
