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

protected:
    shared_ptr<GameObject> GetSharedPtr()
    {
        return static_pointer_cast<GameObject>(shared_from_this());
    }

protected:
    ComPtr<Device> _device = { nullptr };
    ComPtr<DeviceContext> _context = { nullptr };

protected:
    shared_ptr<Transform> _transformCom;


public:
    virtual shared_ptr<GameObject> Clone(any arg) abstract;
    virtual void Free() override;
};

NS_END
