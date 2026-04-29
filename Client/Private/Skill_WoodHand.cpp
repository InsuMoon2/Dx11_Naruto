#include "pch.h"
#include "Skill_WoodHand.h"

#include "Animation.h"
#include "AnimationStateComponent.h"
#include "Bounding_Sphere.h"
#include "Character.h"
#include "Collider.h"
#include "GameObject_Factory.h"
#include "MeshDebrisObject.h"
#include "Model.h"
#include "Shader.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_WoodHand, Protocol::OBJECT_TYPE_SKILL_WOOD_HAND, "SkillSpawn");

Skill_WoodHand::Skill_WoodHand(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_WoodHand::Skill_WoodHand(const Skill_WoodHand& rhs)
    : SkillObject(rhs)
    , _shader(rhs._shader)
    , _leftModel(rhs._leftModel)
    , _rightModel(rhs._rightModel)
    , _leftAnimState(rhs._leftAnimState)
    , _rightAnimState(rhs._rightAnimState)
    , _leftModelTag(rhs._leftModelTag)
    , _rightModelTag(rhs._rightModelTag)
    , _leftAnimStateKey(rhs._leftAnimStateKey)
    , _rightAnimStateKey(rhs._rightAnimStateKey)
    , _leftStartAnimationName(rhs._leftStartAnimationName)
    , _leftEndAnimationName(rhs._leftEndAnimationName)
    , _rightStartAnimationName(rhs._rightStartAnimationName)
    , _rightEndAnimationName(rhs._rightEndAnimationName)
    , _handSpacing(rhs._handSpacing)
    , _forwardOffset(rhs._forwardOffset)
    , _handScale(rhs._handScale)
    , _riseStartOffsetY(rhs._riseStartOffsetY)
    , _riseDuration(rhs._riseDuration)
    , _impactDelayAfterRise(rhs._impactDelayAfterRise)
    , _impactEffectName(rhs._impactEffectName)
    , _impactActiveDuration(rhs._impactActiveDuration)
    , _impactRadius(rhs._impactRadius)
    , _impactEffectScale(rhs._impactEffectScale)
    , _impactSmokeEffectName(rhs._impactSmokeEffectName)
    , _impactSmokeEffectScale(rhs._impactSmokeEffectScale)
    , _impactDebrisEffectName(rhs._impactDebrisEffectName)
    , _impactDebrisCountPerBurst(rhs._impactDebrisCountPerBurst)
    , _impactDebrisScatterRadius(rhs._impactDebrisScatterRadius)
{
}

HRESULT Skill_WoodHand::Initialize_Prototype()
{
    _lifetime = 5.f;
    _hitInterval = 9999.f;
    _maxHitCount = 1;
    _hitLaunchForce = 0.f;
    _collisionPreset = Collision_Preset::Monster_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_WoodHand::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);

    _elapsedTime = 0.f;
    _skillElapsedTime = 0.f;
    _impactSpawned = false;
    _impactActiveRemain = 0.f;

    if (desc)
    {
        if (desc->lifetime > 0.f)
            _lifetime = desc->lifetime;

        _ownerSkillId = desc->ownerSkillId;
        _effectAssetName = desc->effectAssetName;
        _attachOffset = desc->attachOffset;
        _useHitReactionOverride = desc->useHitReactionOverride;
        _hitReactionType = desc->hitReactionType;
        _useLaunchOverride = desc->useLaunchOverride;
        _launchPower = desc->launchPower;
        _launchUp = desc->launchUp;

        if (_transformCom)
        {
            _transformCom->Set_LocalPosition(desc->spawnPosition);
            _transformCom->Set_WorldRotation(desc->spawnRotation.x, desc->spawnRotation.y, desc->spawnRotation.z);
            _transformCom->Set_LocalScale(desc->scale);

            if (desc->useDirectionLookAt && desc->direction.LengthSquared() > 0.001f)
                _transformCom->LookAt(desc->spawnPosition + desc->direction);
        }

        if (desc->ownerObject)
            Set_Owner(desc->ownerObject);
    }

    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(Ready_AnimState(), E_FAIL);

    if (_leftAnimState)
        _leftAnimState->Play_State(_leftAnimStateKey);

    if (_rightAnimState)
        _rightAnimState->Play_State(_rightAnimStateKey);

    if (_leftModel)
        _leftModel->Play_Animation(0.f, false);

    if (_rightModel)
        _rightModel->Play_Animation(0.f, false);

    return S_OK;
}

void Skill_WoodHand::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;

    _skillElapsedTime += timeDelta;

    Update_AnimationPhase(timeDelta);

    if (!_impactSpawned && _skillElapsedTime >= (_riseDuration + _impactDelayAfterRise))
    {
        Spawn_Impact();
    }

    if (_impactActiveRemain > 0.f)
    {
        _impactActiveRemain -= timeDelta;

        if (_impactActiveRemain <= 0.f && _collider)
        {
            _collider->Set_IsActive(false);
        }
    }

    const bool leftFinished = !_leftAnimState || _leftAnimState->Is_CurrentStateSequenceFinished();
    const bool rightFinished = !_rightAnimState || _rightAnimState->Is_CurrentStateSequenceFinished();

    if (leftFinished && rightFinished)
    {
        Set_Destroy(true);
    }
}

void Skill_WoodHand::Late_Update(float timeDelta)
{
    SkillObject::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT Skill_WoodHand::Render()
{
    GameObject::Render();

    if (!_shader)
        return S_OK;

    CHECK_FAILED(Render_Model(_leftModel, Build_HandWorldMatrix(true)), E_FAIL);
    CHECK_FAILED(Render_Model(_rightModel, Build_HandWorldMatrix(false)), E_FAIL);

    return S_OK;
}

HRESULT Skill_WoodHand::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    const uint32 leftModelKey = static_cast<uint32>(std::hash<string>{}(_leftModelTag));
    const uint32 rightModelKey = static_cast<uint32>(std::hash<string>{}(_rightModelTag));

    CHECK_FAILED(Add_Component(leftModelKey, _leftModel), E_FAIL);
    CHECK_NULL(_leftModel, E_FAIL);

    CHECK_FAILED(Add_Component(rightModelKey, _rightModel), E_FAIL);
    CHECK_NULL(_rightModel, E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _leftAnimState), E_FAIL);
    CHECK_NULL(_leftAnimState, E_FAIL);

    Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
    sphereDesc.center = Vec3(0.f, 0.f, _forwardOffset);
    sphereDesc.radius = _impactRadius;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, _collider, &sphereDesc), E_FAIL);
    CHECK_NULL(_collider, E_FAIL);
    _collider->Set_CollisionPreset(_collisionPreset);
    _collider->Set_IsActive(false);

    Shared<Component> rightAnimComponent = GAME->Clone_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE);
    CHECK_NULL(rightAnimComponent, E_FAIL);

    _rightAnimState = dynamic_pointer_cast<AnimationStateComponent>(rightAnimComponent);
    CHECK_NULL(_rightAnimState, E_FAIL);

    const uint32 rightAnimStateKey = static_cast<uint32>(std::hash<string>{}(_rightAnimStateKey + "_AnimState"));
    CHECK_FAILED(Add_Component(rightAnimStateKey, rightAnimComponent), E_FAIL);

    return S_OK;
}

HRESULT Skill_WoodHand::Ready_AnimState()
{
    if (!_leftModel || !_rightModel || !_leftAnimState || !_rightAnimState)
        return E_FAIL;

    vector<Shared<Animation>> leftAnimations;

    if (auto startAnim = GAME->Get_Animation(_leftStartAnimationName))
        leftAnimations.push_back(startAnim);
    else
        LOG_WARN("Skill_WoodHand: left start animation not found - {}", _leftStartAnimationName);

    if (auto endAnim = GAME->Get_Animation(_leftEndAnimationName))
        leftAnimations.push_back(endAnim);
    else
        LOG_WARN("Skill_WoodHand: left end animation not found - {}", _leftEndAnimationName);

    _leftModel->Set_Animations(leftAnimations);
    _leftAnimState->Set_Model(_leftModel);

    FStateAnimationDesc& leftDesc = _leftAnimState->Edit_State(_leftAnimStateKey);
    leftDesc.mode = EStateAnimationMode::Sequence;

    leftDesc.start.animationName = _leftStartAnimationName;
    leftDesc.start.loop = false;
    leftDesc.start.playRate = 0.32f;

    leftDesc.loop.animationName.clear();
    leftDesc.loop.loop = false;
    leftDesc.loop.playRate = 1.f;

    leftDesc.end.animationName = _leftEndAnimationName;
    leftDesc.end.loop = false;
    leftDesc.end.playRate = 1.f;

    vector<Shared<Animation>> rightAnimations;

    if (auto startAnim = GAME->Get_Animation(_rightStartAnimationName))
        rightAnimations.push_back(startAnim);
    else
        LOG_WARN("Skill_WoodHand: right start animation not found - {}", _rightStartAnimationName);

    if (auto endAnim = GAME->Get_Animation(_rightEndAnimationName))
        rightAnimations.push_back(endAnim);
    else
        LOG_WARN("Skill_WoodHand: right end animation not found - {}", _rightEndAnimationName);

    _rightModel->Set_Animations(rightAnimations);
    _rightAnimState->Set_Model(_rightModel);

    FStateAnimationDesc& rightDesc = _rightAnimState->Edit_State(_rightAnimStateKey);
    rightDesc.mode = EStateAnimationMode::Sequence;

    rightDesc.start.animationName = _rightStartAnimationName;
    rightDesc.start.loop = false;
    rightDesc.start.playRate = 0.32f;

    rightDesc.loop.animationName.clear();
    rightDesc.loop.loop = false;
    rightDesc.loop.playRate = 1.f;

    rightDesc.end.animationName = _rightEndAnimationName;
    rightDesc.end.loop = false;
    rightDesc.end.playRate = 1.f;

    return S_OK;
}

void Skill_WoodHand::Update_AnimationPhase(float timeDelta)
{
    if (_leftModel)
        _leftModel->Play_Animation(timeDelta, true);

    if (_rightModel)
        _rightModel->Play_Animation(timeDelta, true);
}

void Skill_WoodHand::Spawn_Impact()
{
    _impactSpawned = true;
    _impactActiveRemain = _impactActiveDuration;

    if (_collider)
    {
        _collider->Set_IsActive(true);
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
    }

    Spawn_Effect_Once(_impactEffectName, Resolve_ImpactWorldPosition(), _impactEffectScale);
    Spawn_ImpactSmokeBurst();
    Spawn_ImpactDebrisBurst();
    GAME->Play_Sound(L"WoodHand_Clap.wav", ESoundChannel::Effect, 0.3f);
}

void Skill_WoodHand::Spawn_ImpactSmokeBurst()
{
    const Vec3 impactCenter = Resolve_ImpactWorldPosition();

    Spawn_Effect_Once(_impactSmokeEffectName, impactCenter, _impactSmokeEffectScale);
    Spawn_Effect_Once(_impactSmokeEffectName, Resolve_HandSmokeWorldPosition(true), _impactSmokeEffectScale);
    Spawn_Effect_Once(_impactSmokeEffectName, Resolve_HandSmokeWorldPosition(false), _impactSmokeEffectScale);
}

void Skill_WoodHand::Spawn_ImpactDebrisBurst()
{
    const array<Vec3, 3> burstCenters =
    {
        Resolve_ImpactWorldPosition(),
        Resolve_HandSmokeWorldPosition(true),
        Resolve_HandSmokeWorldPosition(false),
    };

    const int32 safeBurstCount = max(1, _impactDebrisCountPerBurst);

    for (const Vec3& burstCenter : burstCenters)
    {
        for (int32 debrisIndex = 0; debrisIndex < safeBurstCount; ++debrisIndex)
        {
            const float angle = XM_2PI * (static_cast<float>(debrisIndex) / static_cast<float>(safeBurstCount));
            const Vec3 radialDir = Vec3(cosf(angle), 0.f, sinf(angle));
            const float radiusJitter = Utils::RandomRange(0.25f, 1.0f);

            MeshDebrisObject::FMeshDebrisDesc debrisDesc{};
            debrisDesc.effectAssetName = _impactDebrisEffectName;
            debrisDesc.position = burstCenter + radialDir * (_impactDebrisScatterRadius * radiusJitter);
            debrisDesc.spawnRotation = Vec3(
                Utils::RandomRange(0.f, 360.f),
                Utils::RandomRange(0.f, 360.f),
                Utils::RandomRange(0.f, 360.f));
            debrisDesc.spawnScale = Vec3(1.f, 1.f, 1.f);

            const float randomScale = Utils::RandomRange(0.10f, 0.18f);
            debrisDesc.effectLocalScale = Vec3(randomScale, randomScale, randomScale);

            debrisDesc.initialVelocity = Vec3(
                radialDir.x * Utils::RandomRange(3.5f, 6.5f),
                Utils::RandomRange(7.f, 11.f),
                radialDir.z * Utils::RandomRange(3.5f, 6.5f));

            debrisDesc.gravity = -24.f;
            debrisDesc.angularVelocityDeg = Vec3(
                Utils::RandomRange(-420.f, 420.f),
                Utils::RandomRange(-420.f, 420.f),
                Utils::RandomRange(-420.f, 420.f));
            debrisDesc.lifetime = Utils::RandomRange(0.8f, 1.2f);
            debrisDesc.groundY = burstCenter.y;
            debrisDesc.destroyOnGroundHit = false;
            debrisDesc.stopOnGroundHit = true;

            GAME->Clone_And_Add_GameObject(
                ETOI(ELevelType::Static),
                Protocol::OBJECT_TYPE_MESH_DEBRIS,
                GAME->Current_Level(),
                TEXT("Layer_Effect"),
                &debrisDesc);
        }
    }
}

Matrix Skill_WoodHand::Build_HandWorldMatrix(bool leftHand) const
{
    if (!_transformCom)
        return Matrix::Identity;

    const float side = leftHand ? -1.f : 1.f;
    const float yawDegrees = leftHand ? 90.f : 0.f; // 왼손 : 오른손
    float riseOffsetY = 0.f;

    if (_skillElapsedTime < _riseDuration)
    {
        const float riseAlpha = std::clamp(_skillElapsedTime / max(_riseDuration, 0.001f), 0.f, 1.f);
        const float easedRiseAlpha = riseAlpha * riseAlpha * (3.f - 2.f * riseAlpha);
        riseOffsetY = -_riseStartOffsetY * (1.f - easedRiseAlpha);
    }

    const Vec3 localOffset = Vec3(side * _handSpacing, riseOffsetY - 1.6f, _forwardOffset);

    return Matrix::CreateScale(_handScale) *
        Matrix::CreateRotationY(XMConvertToRadians(yawDegrees)) *
        Matrix::CreateTranslation(localOffset) *
        _transformCom->Get_WorldMatrix();
}

Vec3 Skill_WoodHand::Resolve_ImpactWorldPosition() const
{
    if (!_transformCom)
        return Vec3::Zero;

    return _transformCom->Get_WorldPosition() + _transformCom->Get_WorldForward() * _forwardOffset;
}

Vec3 Skill_WoodHand::Resolve_HandSmokeWorldPosition(bool leftHand) const
{
    Matrix handWorldMatrix = Build_HandWorldMatrix(leftHand);
    Vec3 handScale = Vec3::One;
    Vec3 handPosition = handWorldMatrix.Translation();
    Quat handRotation = Quat::Identity;

    if (handWorldMatrix.Decompose(handScale, handRotation, handPosition) == false)
        handPosition = handWorldMatrix.Translation();

    if (_transformCom)
        handPosition.y = _transformCom->Get_WorldPosition().y;

    return handPosition;
}

HRESULT Skill_WoodHand::Render_Model(Shared<Model> model, const Matrix& worldMatrix)
{
    if (!model)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(worldMatrix), E_FAIL);

    const Vec4 outlineColor = Vec4(0.04f, 0.03f, 0.02f, 1.f);
    const float outlineThickness = 0.0035f;

    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);

    const size_t numMeshes = model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(model->Bind_BoneMatrices(_shader, "g_BoneMatrices"), E_FAIL);
        model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT Skill_WoodHand::Bind_ShaderResources(const Matrix& worldMatrix)
{
    CHECK_NULL(_shader, E_FAIL);

    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

void Skill_WoodHand::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy() || !_impactSpawned || _impactActiveRemain <= 0.f)
        return;

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner || otherOwner == Get_Owner())
        return;

    if (_hitCount >= _maxHitCount)
        return;

    if (_hitCooldowns[otherOwner.get()] > 0.f)
        return;

    auto character = dynamic_cast<Character*>(otherOwner.get());
    if (!character)
        return;

    if (!Apply_Skill_Hit(character, 10.f, _hitLaunchForce, 0.f))
        return;

    _hitCooldowns[otherOwner.get()] = (_hitInterval > 0.f) ? _hitInterval : 9999.f;
    ++_hitCount;
}

Shared<GameObject> Skill_WoodHand::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_WoodHand>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_WoodHand");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_WoodHand::Clone(void* arg)
{
    auto clone = make_shared<Skill_WoodHand>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_WoodHand");
        return nullptr;
    }

    return clone;
}

void Skill_WoodHand::Free()
{
    SkillObject::Free();
}
