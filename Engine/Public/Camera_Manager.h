#pragma once

#include "Base.h"
#include "Delegate.h"

NS_BEGIN(Engine)
class Camera;

class ENGINE_DLL Camera_Manager : public Base
{
public:
    explicit Camera_Manager() = default;
    virtual ~Camera_Manager() = default;

public:
    void            Update(float timeDelta);

    void            Set_ActiveCamera(Shared<Camera> camera);
    Shared<Camera>  Get_ActiveCamer() const { return _activeCamera.lock(); }

    bool            Is_ActiveCamera(const Shared<Camera> camera) const;

    void            Toggle_Camera();
    void            Register_Camera(Shared<Camera> camera);

    Shared<Camera>  Find_Camera(Protocol::OBJECT_TYPE type);

private:
    vector<Weak<Camera>> _cameras;
    Weak<Camera>         _activeCamera;

public:
    static Unique<Camera_Manager> Create();
    virtual void Free() override;

};

NS_END
