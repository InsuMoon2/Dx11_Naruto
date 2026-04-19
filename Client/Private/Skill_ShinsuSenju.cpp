#include "pch.h"
#include "Skill_ShinsuSenju.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Shader.h"
#include "Model.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ShinsuSenju, Protocol::OBJECT_TYPE_SHINSUSENJU, "SkillSpawn");

Skill_ShinsuSenju::Skill_ShinsuSenju(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_ShinsuSenju::Skill_ShinsuSenju(const Skill_ShinsuSenju& rhs)
    : SkillObject(rhs)
    , _shader(rhs._shader)
    , _model(rhs._model)
    , _burstInterval(rhs._burstInterval)
    , _maxBurstCount(rhs._maxBurstCount)
{

}

HRESULT Skill_ShinsuSenju::Initialize_Prototype()
{
    _lifetime        = 5.f;

    _colliderRadius  = 0.f;
    _collisionPreset = Collision_Preset::Enviroment;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_ShinsuSenju::Initialize(void* arg)
{
    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    _delayElapsed = 0.f;
    _burstElapsed = 0.f;
    _burstCount = 0;
    _isBurstFinished = false;

    if (_transformCom)
    {
        auto ownerPos = Get_Owner()->Get_Transform()->Get_WorldPosition();
        auto ownerForward = Get_Owner()->Get_Transform()->Get_WorldForward();
        auto ownerRot = Get_Owner()->Get_Transform()->Get_WorldRotation();

        const Vec3 spawnPos = ownerPos - ownerForward * 28.f + Vec3(0.f, 20.f, 0.f);

        _transformCom->Set_WorldPosition(spawnPos);
        _transformCom->LookAt(Get_Owner()->Get_Transform()->Get_WorldPosition());

        const Quat yawOffset = Quat::CreateFromAxisAngle(Vec3::Up, XMConvertToRadians(180.f));
        _transformCom->Add_LocalRotation(yawOffset);

   
        _transformCom->Set_LocalScale(0.0005f, 0.0005f, 0.0005f);
    }

    if (_collider)
    {
        _collider->Set_IsActive(false);
    }

    //GAME->Play_Cinematic(L"Skill_ShinsuSenju", Get_Owner()->Get_Transform(), true);

    return S_OK;
}

void Skill_ShinsuSenju::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;


}

void Skill_ShinsuSenju::Late_Update(float timeDelta)
{
    SkillObject::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT Skill_ShinsuSenju::Render()
{
    GameObject::Render();

    if (!_shader || !_model)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);
    const float outlineThickness = 0.0035f;

    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_model->Bind_BoneMatrices(_shader, "g_BoneMatrices"), E_FAIL);
        _model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

HRESULT Skill_ShinsuSenju::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    const uint32 modelKey = static_cast<uint32>(std::hash<string>{}("ShinsuSenju"));
    CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);

    return S_OK;
}

HRESULT Skill_ShinsuSenju::Bind_ShaderResources()
{
    CHECK_NULL(_shader, E_FAIL);
    CHECK_NULL(_transformCom, E_FAIL);

    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

void Skill_ShinsuSenju::Spawn_ShinsuSenju(const Vec3 targetPos)
{

}

void Skill_ShinsuSenju::Spawn_Arm(const Vec3& targetDirection)
{
}


Shared<GameObject> Skill_ShinsuSenju::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_ShinsuSenju>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_ShinsuSenju");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_ShinsuSenju::Clone(void* arg)
{
    auto clone = make_shared<Skill_ShinsuSenju>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_ShinsuSenju");

        return nullptr;
    }

    return clone;
}

void Skill_ShinsuSenju::Free()
{
    SkillObject::Free();
}
