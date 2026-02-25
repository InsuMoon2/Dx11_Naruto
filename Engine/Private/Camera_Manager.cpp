#include "pch.h"
#include "Camera_Manager.h"
#include "Input_Manager.h"

void Camera_Manager::Update(float timeDelta)
{
    if (INPUT->KeyDown(KEY_TYPE::F8))
        Toggle_Camera();
}

void Camera_Manager::Set_ActiveCamera(Shared<Camera> camera)
{
    _activeCamera = camera;

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
            _activeCamera = _cameras[next];

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

Unique<Camera_Manager> Camera_Manager::Create()
{
    auto instance = make_unique<Camera_Manager>();

    return instance;
}

void Camera_Manager::Free()
{
    Base::Free();
}
