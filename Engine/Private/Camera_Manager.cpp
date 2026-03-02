#include "pch.h"
#include "Camera_Manager.h"

#include "Camera.h"
#include "Input_Manager.h"

void Camera_Manager::Update(float timeDelta)
{
    if (INPUT->KeyDown(KEY_TYPE::F8))
        Toggle_Camera();
}

void Camera_Manager::Set_ActiveCamera(Shared<Camera> camera)
{
    _activeCamera = camera;

    if (camera->Get_ObjectType() == Protocol::OBJECT_TYPE_CAMERA_TARGET)
        INPUT->LockMouse();
    else
        INPUT->UnlockMouse();

    LOG_INFO("Active Camera Changed");
}

bool Camera_Manager::Is_ActiveCamera(const Shared<Camera> camera) const
{
    return _activeCamera.lock() == camera;
}

void Camera_Manager::Toggle_Camera()
{
    auto current = _activeCamera.lock();

    for (size_t i = 0; i < _cameras.size(); i++)
    {
        if (_cameras[i].lock() == current)
        {
            size_t next = (i + 1) % _cameras.size();
            auto nextCam = _cameras[next].lock();

            if (current && nextCam)
            {
                auto srcT = current->Get_Component<Transform>();
                auto destT = nextCam->Get_Component<Transform>();
                if (srcT && destT)
                {
                    destT->Set_LocalPosition(srcT->Get_WorldPosition());
                    destT->Set_LocalRotation(srcT->Get_WorldRotation());
                }
            }

            Set_ActiveCamera(nextCam);

            LOG_INFO("Camera Toggled");
            return;
        }
    }
}

void Camera_Manager::Register_Camera(Shared<Camera> camera)
{
    _cameras.push_back(camera);

    // 첫번째 카메라 액티브로 세팅
    if (!_activeCamera.lock())
        _activeCamera = camera;
}

Shared<Camera> Camera_Manager::Find_Camera(Protocol::OBJECT_TYPE type)
{
    for (auto& weak : _cameras)
    {
        auto camera = weak.lock();

        if (camera && camera->Get_ObjectType() == type)
        {
            return camera;
        }
    }

    return nullptr;
}

Unique<Camera_Manager> Camera_Manager::Create()
{
    auto instance = make_unique<Camera_Manager>();

    return instance;
}

void Camera_Manager::Free()
{
    Base::Free();
}
