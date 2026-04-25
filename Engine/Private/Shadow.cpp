#include "pch.h"
#include "Shadow.h"
#include "Shader.h"

HRESULT Shadow::Update_LightDesc(const FLightDesc& lightDesc)
{
    _lightDesc = lightDesc;
    _hasLight = lightDesc.castShadow && lightDesc.type == ELightType::Directional;

    if (!_hasLight)
    {
        _viewMatrix = Matrix::Identity;
        _projMatrix = Matrix::Identity;
        return S_FALSE;
    }

    Vec3 lightDir = Vec3(lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z);
    if (lightDir.LengthSquared() <= FLT_EPSILON)
        lightDir = Vec3::Down;
    else
        lightDir.Normalize();

    Vec3 upAxis = Vec3::Up;
    if (fabsf(lightDir.Dot(upAxis)) > 0.98f)
        upAxis = Vec3::Forward;

    Vec3 shadowEye = Vec3::Zero; 
    Vec3 shadowTarget = Vec3::Zero; 
    float shadowAspect = 1.f; 
    float shadowFovY = XMConvertToRadians(60.f);
    bool usePerspectiveShadowCamera = lightDesc.useShadowCamera; // true일 때만 직접 배치한 perspective shadow camera를 사용한다.

    if (lightDesc.useShadowCamera)
    {
        shadowEye = lightDesc.shadowEye;
        shadowTarget = lightDesc.shadowTarget;
        shadowAspect = (lightDesc.shadowAspect > 0.f)
            ? lightDesc.shadowAspect
            : (max(lightDesc.shadowOrthoWidth, 1.f) / max(lightDesc.shadowOrthoHeight, 1.f));
        shadowFovY = max(lightDesc.shadowFovY, XMConvertToRadians(5.f));
    }
    else
    {
        const Vec3 focus = lightDesc.shadowCenter;
        const float eyeDistance = max(lightDesc.shadowFar * 0.5f, lightDesc.shadowNear + 1.f);
        const float halfHeight = max(lightDesc.shadowOrthoHeight * 0.5f, 1.f);

        shadowEye = focus - lightDir * eyeDistance;
        shadowTarget = focus;
        shadowAspect = max(lightDesc.shadowOrthoWidth, 1.f) / max(lightDesc.shadowOrthoHeight, 1.f);
        shadowFovY = max(2.f * atanf(halfHeight / eyeDistance), XMConvertToRadians(5.f));
    }

    if (Vec3::DistanceSquared(shadowEye, shadowTarget) <= 0.0001f)
        shadowTarget = shadowEye + lightDir;

    const float shadowNear = max(lightDesc.shadowNear, 0.01f); 
    const float shadowFar = max(lightDesc.shadowFar, shadowNear + 1.f);

    _viewMatrix = XMMatrixLookAtLH(shadowEye, shadowTarget, upAxis);

    if (usePerspectiveShadowCamera)
    {
        _projMatrix = XMMatrixPerspectiveFovLH(
            shadowFovY,
            max(shadowAspect, 0.01f),
            shadowNear,
            shadowFar);
    }
    else
    {
        // Directional light shadow는 FOV가 아니라 일정한 texel 밀도를 가진 orthographic projection이 기본이다.
        _projMatrix = XMMatrixOrthographicLH(
            max(lightDesc.shadowOrthoWidth, 1.f),
            max(lightDesc.shadowOrthoHeight, 1.f),
            shadowNear,
            shadowFar);
    }

    return S_OK;
}

HRESULT Shadow::Bind_TransformStateMatrix(Shared<Shader> shader, const char* viewName, const char* projName)
{
    CHECK_NULL(shader, E_FAIL);
    CHECK_FAILED(shader->Bind_Matrix(viewName, &_viewMatrix), E_FAIL);
    CHECK_FAILED(shader->Bind_Matrix(projName, &_projMatrix), E_FAIL);

    return S_OK;
}

void Shadow::Clear()
{
    _lightDesc = {};
    _hasLight = false;
    _viewMatrix = Matrix::Identity;
    _projMatrix = Matrix::Identity;
}

Unique<Shadow> Shadow::Create()
{
    return make_unique<Shadow>();
}

void Shadow::Free()
{
    Base::Free();
}
