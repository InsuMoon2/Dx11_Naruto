#include "pch.h"
#include "Weapon.h"
#include "Model.h"
#include "Shader.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(Weapon, Protocol::OBJECT_TYPE_PART_OBJECT)

Weapon::Weapon(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : PartObject(device, context)
{
}

Weapon::Weapon(const Weapon& rhs)
    : PartObject(rhs)
{
}

HRESULT Weapon::Initialize_Prototype()
{
    return PartObject::Initialize_Prototype();
}

HRESULT Weapon::Initialize(void* arg)
{
    CHECK_FAILED(PartObject::Initialize(arg), E_FAIL);

    FWeaponDesc* desc = static_cast<FWeaponDesc*>(arg);
    wstring modelTag = L"";

    if (desc)
    {
        modelTag = desc->modelAssetTag;
        _socketMatrix = desc->socketMatrix;
    }

    CHECK_FAILED(Ready_Components(modelTag), E_FAIL);

    return S_OK;
}

void Weapon::Priority_Update(float timeDelta)
{
    PartObject::Priority_Update(timeDelta);
}

void Weapon::Update(float timeDelta)
{
    PartObject::Update(timeDelta);

    Matrix socketMatrix = Matrix::Identity;

    if (_socketMatrix)
    {
        socketMatrix = *_socketMatrix;

        // 부모의 Scale이 영향없게 세팅
        Vec3 right = socketMatrix.Right();
        Vec3 up = socketMatrix.Up();
        Vec3 backward = socketMatrix.Backward();

        right.Normalize();
        up.Normalize();
        backward.Normalize();

        socketMatrix.Right(right);
        socketMatrix.Up(up);
        socketMatrix.Backward(backward);
    }

    Update_CombinedWorldMatrix(Get_Transform()->Get_WorldMatrix() * socketMatrix);
}

void Weapon::Late_Update(float timeDelta)
{
    PartObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Weapon::Render()
{
    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    if (!_model)
        return S_OK;

    size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; i++)
    {
        _model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

HRESULT Weapon::Ready_Components(const wstring& modelAssetTag)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shader), E_FAIL);

    if (!modelAssetTag.empty())
    {
        string tagStr = Utils::ToString(modelAssetTag);
        uint32 modelKey = static_cast<uint32>(std::hash<string>{}(tagStr));
        CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);
    }

    return S_OK;
}

HRESULT Weapon::Bind_ShaderResources()
{
    _shader->Bind_Matrix("g_WorldMatrix", &_combinedWorldMatrix);
    _shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    GAME->Bind_CamPosition(_shader, "g_vCamPosition");
    CHECK_FAILED(Bind_Lights(), E_FAIL);

    return S_OK;
}

HRESULT Weapon::Bind_Lights()
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

Shared<GameObject> Weapon::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Weapon>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Weapon");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Weapon::Clone(void* arg)
{
    auto clone = make_shared<Weapon>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Weapon");

        return nullptr;
    }

    return clone;
}

void Weapon::Free()
{
    PartObject::Free();
}
