#include "pch.h"
#include "Editor_Camera_Free.h"

Editor_Camera_Free::Editor_Camera_Free(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera_Free(device, context)
{
}

Editor_Camera_Free::Editor_Camera_Free(const Editor_Camera_Free& rhs)
    : Camera_Free(rhs)
{
}

HRESULT Editor_Camera_Free::Initialize_Prototype()
{
    Camera_Free::Initialize_Prototype();

    return S_OK;
}

HRESULT Editor_Camera_Free::Initialize(void* arg)
{
    Camera_Free::Initialize(arg);


    return S_OK;
}

void Editor_Camera_Free::Priority_Update(float timeDelta)
{
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;

        if (delta.x != 0)
            _transformCom->Rotate_Axis(Vec3::Up, delta.x * _mouseSensor);

        if (delta.y != 0)
            _transformCom->Rotate_Axis(_transformCom->Get_WorldRight(), delta.y * _mouseSensor);

        if (ImGui::IsKeyDown(ImGuiKey_W))
            _transformCom->Move_Backward(timeDelta);

        if (ImGui::IsKeyDown(ImGuiKey_S))
            _transformCom->Move_Forward(timeDelta);

        if (ImGui::IsKeyDown(ImGuiKey_A))
            _transformCom->Move_Left(timeDelta);

        if (ImGui::IsKeyDown(ImGuiKey_D))
            _transformCom->Move_Right(timeDelta);
    }

    float wheel = ImGui::GetIO().MouseWheel;

    if (wheel != 0.f)
        _transformCom->Move_Backward(wheel * 0.1f);
}

Matrix Editor_Camera_Free::Get_ViewMatrix() const
{
    return _transformCom->Get_WorldMatrix().Invert();
}

Shared<Editor_Camera_Free> Editor_Camera_Free::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Editor_Camera_Free>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Editor_Camera_Free");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Editor_Camera_Free::Clone(void* arg)
{
    auto clone = make_shared<Editor_Camera_Free>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Editor_Camera_Free");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Editor_Camera_Free::Free()
{
    Camera_Free::Free();
}
