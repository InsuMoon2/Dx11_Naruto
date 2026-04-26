#include "pch.h"
#include "UI_Targeting.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "TargetComponent.h"
#include "Monster.h"
#include "Transform.h"
#include "Model.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_Targeting, Protocol::OBJECT_TYPE_UI_TARGETING)
IMPLEMENT_REFLECTION(UI_Targeting)

bool UI_Targeting::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_Targeting";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    return true;
}

UI_Targeting::UI_Targeting(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_Targeting::UI_Targeting(const UI_Targeting& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_Targeting::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_Targeting::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    Set_UIScale(512.f, 512.f);

    Set_Visibility(false);

    return S_OK;
}

void UI_Targeting::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_Targeting::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    if (_targetCom.expired())
    {
        Set_Visibility(false);
        return;
    }

    Shared<TargetComponent> targetCom = _targetCom.lock();

    if (targetCom->IsLockOn() && !targetCom->Get_LockedTarget().expired())
    {
        Shared<Character> lockedMonster = targetCom->Get_LockedTarget().lock();
        auto targetTransform = lockedMonster->Get_Transform();

        auto modelCom = lockedMonster->Get_Component<Model>();

        Vec3 targetWorldPos = targetTransform->Get_WorldPosition(); 

        if (modelCom)
        {
            const Matrix* socketMatrixPtr = modelCom->Get_SocketBoneMatrixPtr("Spine1"); 
            if (socketMatrixPtr)
            {
                Matrix combinedMatrix = (*socketMatrixPtr) * targetTransform->Get_WorldMatrix();
                targetWorldPos = combinedMatrix.Translation();
            }
            else
            {
                targetWorldPos.y += 1.0f;
            }
        }
        else
        {
            targetWorldPos.y += 1.0f;
        }

        Matrix viewMatrix = *GAME->Get_Transform(ETransformState::View);
        Matrix projMatrix = *GAME->Get_Transform(ETransformState::Proj);

        const float uiViewportWidth = GAME->Get_UIViewportWidth();
        const float uiViewportHeight = GAME->Get_UIViewportHeight();

        DirectX::SimpleMath::Viewport mathViewport(
            0.f, 0.f, uiViewportWidth, uiViewportHeight, 0.f, 1.f);

        Vec3 screenPos = mathViewport.Project(
            targetWorldPos,
            projMatrix,
            viewMatrix,
            Matrix::Identity);

        if (screenPos.z >= 1.f || screenPos.z < 0.f)
        {
            Set_Visibility(false);
        }
        else
        {
            Set_Visibility(true);

            float uiScale = GAME->Get_UIScale();
            Vec2  uiOffset = GAME->Get_UIViewportOffset();

            float uiX = (screenPos.x - uiOffset.x) / uiScale;
            float uiY = (screenPos.y - uiOffset.y) / uiScale;

            if (uiScale <= 0.f)
            {
                Set_Visibility(false);
                return;
            }

            const float uiPosX = (screenPos.x - uiOffset.x) / uiScale;
            const float uiPosY = (screenPos.y - uiOffset.y) / uiScale;

            Set_UIPosition(uiPosX, uiPosY);
        }
    }
    else
    {
        Set_Visibility(false);
    }

    __super::Update_Transform();
}


void UI_Targeting::Late_Update(float timeDelta)
{
    if (!Is_VisibleForRender()) return;

    // 렌더 그룹 등록은 UI_Manager가 담당하므로 여기서는 UI 자체 Late_Update만 수행한다.
    UIObject::Late_Update(timeDelta);
}

HRESULT UI_Targeting::Render()
{
    if (!Is_VisibleForRender()) return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", ETOI(ETargetingTexture::TargetBase)), E_FAIL);

    Vec4 targetColor = { 0.f, 1.f, 0.f, 1.f };
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &targetColor, sizeof(Vec4)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(7), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_Targeting::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_TARGET, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_Targeting> UI_Targeting::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_Targeting>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : UI_Targeting");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_Targeting::Clone(void* arg)
{
    auto clone = make_shared<UI_Targeting>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_Targeting");

        return nullptr;
    }

    return clone;
}

void UI_Targeting::Free()
{
    _targetCom.reset();

    UIObject::Free();
}
