#include "pch.h"
#include "Player_BodyUpper.h"

#include "Model.h"
#include "Shader.h"

Player_BodyUpper::Player_BodyUpper(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : PartObject(device, context)
{

}

Player_BodyUpper::Player_BodyUpper(const Player_BodyUpper& rhs)
    : PartObject(rhs)
{

}

HRESULT Player_BodyUpper::Initialize_Prototype()
{
    CHECK_FAILED(PartObject::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT Player_BodyUpper::Initialize(void* arg)
{
    CHECK_FAILED(PartObject::Initialize(arg), E_FAIL);

    FPartObjectDesc* desc = static_cast<FPartObjectDesc*>(arg);

    wstring modelTag = L"";

    if (desc)
    {
        modelTag = desc->modelAssetTag;
        _masterPoseModel = desc->masterPoseModel;
    }

    CHECK_FAILED(Ready_Components(modelTag), E_FAIL);

    if (_model && _masterPoseModel)
    {
        _model->Set_MasterPoseModel(_masterPoseModel);
    }

    return S_OK;
}

void Player_BodyUpper::Priority_Update(float timeDelta)
{
    PartObject::Priority_Update(timeDelta);
}

void Player_BodyUpper::Update(float timeDelta)
{
    PartObject::Update(timeDelta);

    // 애니메이션 재생
    _model->Play_Animation(timeDelta, true);

    Update_CombinedWorldMatrix(Get_Transform()->Get_WorldMatrix());
}

void Player_BodyUpper::Late_Update(float timeDelta)
{
    PartObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Player_BodyUpper::Render()
{
    PartObject::Render();

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);
    size_t numMeshes = _model->Get_NumMeshes();
  
    for (size_t i = 0; i < numMeshes; i++)
    {
        CHECK_FAILED(_model->Bind_BoneMatrices(_shader, "g_BoneMatrices"), E_FAIL);

        _model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);
    
        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

HRESULT Player_BodyUpper::Ready_Components(const wstring& modelAssetTag)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    // 전달받은, modelAssetTag 바탕으로 동적 Model 컴포넌트 부착.. 흠
    //CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MODEL_SASKE, _model), E_FAIL);

    string tagStr = Utils::ToString(modelAssetTag);
    uint32 modelKey = static_cast<uint32>(std::hash<string>{}(tagStr));

    CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);

    return S_OK;
}

HRESULT Player_BodyUpper::Bind_ShaderResources()
{
    _shader->Bind_Matrix("g_WorldMatrix", &_combinedWorldMatrix);

    _shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));
    GAME->Bind_CamPosition(_shader, "g_vCamPosition");

    CHECK_FAILED(Bind_Lights(), E_FAIL);

    return S_OK;
}

HRESULT Player_BodyUpper::Bind_Lights()
{
    const FLightDesc* lightDesc = GAME->Get_LightDesc(0);
    FLightDesc defaultLight;
    if (!lightDesc)
    {
        defaultLight.direction = Vec4(0.f, -1.f, 1.f, 0.f);
        defaultLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
        defaultLight.ambient = Vec4(0.4f, 0.4f, 0.4f, 1.f);
        defaultLight.specular = Vec4(1.f, 1.f, 1.f, 1.f);
        lightDesc = &defaultLight;
    }
    if (lightDesc)
    {
        CHECK_FAILED(_shader->Bind_RawValue("g_vLightDir", &lightDesc->direction, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shader->Bind_RawValue("g_vLightDiffuse", &lightDesc->diffuse, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shader->Bind_RawValue("g_vLightAmbient", &lightDesc->ambient, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shader->Bind_RawValue("g_vLightSpecular", &lightDesc->specular, sizeof(Vec4)), E_FAIL);
    }
    return S_OK;
}

Shared<GameObject> Player_BodyUpper::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Player_BodyUpper>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Player_BodyUpper");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Player_BodyUpper::Clone(void* arg)
{
    auto clone = make_shared<Player_BodyUpper>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Player_BodyUpper");

        return nullptr;
    }

    return clone;
}

void Player_BodyUpper::Free()
{
    PartObject::Free();
}
