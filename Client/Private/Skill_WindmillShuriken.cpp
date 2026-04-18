#include "pch.h"
#include "Skill_WindmillShuriken.h"

#include "AnimationStateComponent.h"
#include "GameObject_Factory.h"
#include "Shader.h"
#include "Model.h"
#include "Collider.h"
#include "Animation.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_WindmillShuriken, Protocol::OBJECT_TYPE_SHURIKEN, "SkillSpawn");

Skill_WindmillShuriken::Skill_WindmillShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_WindmillShuriken::Skill_WindmillShuriken(const Skill_WindmillShuriken& rhs)
    : SkillObject_Projectile(rhs)
    , _shader(rhs._shader)
    , _model(rhs._model)
    , _animState(rhs._animState)
    , _animStateKey(rhs._animStateKey)
    , _startAnimationName(rhs._startAnimationName)
    , _loopAnimationName(rhs._loopAnimationName)
{
}

HRESULT Skill_WindmillShuriken::Initialize_Prototype()
{
    _speed = 35.f;
    _maxDistance = 40.f;

    _lifetime = 4.f;
    _maxHitCount = 1;
    _hitLaunchForce = 1.5f;

    _colliderRadius = 2.5f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_WindmillShuriken::Initialize(void* arg)
{
    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(Ready_AnimState(), E_FAIL);

    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _transformCom->Set_LocalScale(Vec3(2.6f, 2.6f, 2.6f));

    _colliderRadius = 20.5f;

    if (_collider)
        _collider->Set_IsActive(IsLaunched());

    if (_animState)
    {
        _animState->Play_State(_animStateKey);

        if (_model)
            _model->Play_Animation(0.f, false);
    }

    return S_OK;
}

void Skill_WindmillShuriken::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

     if (Is_Destroy())
        return;

    if (_model)
        _model->Play_Animation(timeDelta, true);

}

void Skill_WindmillShuriken::Late_Update(float timeDelta)
{
    SkillObject_Projectile::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT Skill_WindmillShuriken::Render()
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

        //CHECK_FAILED(_shader->Begin_Pass(1), E_FAIL);
        //CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

void Skill_WindmillShuriken::Launch(const Vec3& direction)
{
    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Shuriken_Loop";
    playDesc.loopOverride = true;

    CHECK_FAILED(_effectCom->Play_Effect(playDesc));

    if (_transformCom)
    {
        _transformCom->Set_LocalRotation(90.f, 0.f, 0.f);
    }

    SkillObject_Projectile::Launch(direction);

    if (_animState)
    {
        _animState->Play_StateLoopOnly(_animStateKey);
    }

    if (_collider)
    {
        _collider->Set_IsActive(true);
    }
}


HRESULT Skill_WindmillShuriken::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    const string modelTag = "Model_Shuriken";
    const uint32 modelKey = static_cast<uint32>(std::hash<string>{}(modelTag));

    CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);
    if (_model)
    {
        vector<Shared<Animation>> anims;

        if (auto startAnim = GAME->Get_Animation(_startAnimationName))
        {
            anims.push_back(startAnim);
        }

        if (auto loopAnim = GAME->Get_Animation(_loopAnimationName))
        {
            anims.push_back(loopAnim);
        }

        _model->Set_Animations(anims);
    }

    return S_OK;
}

HRESULT Skill_WindmillShuriken::Ready_AnimState()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);
    CHECK_NULL(_animState, E_FAIL);

    FStateAnimationDesc& desc = _animState->Edit_State(_animStateKey);
    desc.mode = EStateAnimationMode::Sequence;

    desc.start.animationName = _startAnimationName;
    desc.start.loop = false;
    desc.start.playRate = 1.2f;

    desc.loop.animationName = _loopAnimationName;
    desc.loop.loop = true;
    desc.loop.playRate = 2.f;

    desc.end.animationName.clear();
    desc.end.loop = false;
    desc.end.playRate = 1.f;

    return S_OK;
}

HRESULT Skill_WindmillShuriken::Bind_ShaderResources()
{
    CHECK_NULL(_shader, E_FAIL);
    CHECK_NULL(_transformCom, E_FAIL);

    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

Shared<GameObject> Skill_WindmillShuriken::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_WindmillShuriken>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Skill_WindmillShuriken");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_WindmillShuriken::Clone(void* arg)
{
    auto clone = make_shared<Skill_WindmillShuriken>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Skill_WindmillShuriken");
        return nullptr;
    }

    return clone;
}

void Skill_WindmillShuriken::Free()
{
    SkillObject_Projectile::Free();
}
