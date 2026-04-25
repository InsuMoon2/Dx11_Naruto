#include "pch.h"
#include "AN_SpawnAttack.h"
#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject.h"
#include "Client_Defines.h"
#include "Model.h"

REGISTER_ANIM_NOTIFY(AN_SpawnAttack)
IMPLEMENT_REFLECTION(AN_SpawnAttack)

bool AN_SpawnAttack::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnAttack";
    info.properties.clear();

    PROPERTY_ENUM_JSON("스킬 오브젝트 타입", "spawn_object_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);
    PROPERTY_STRING_JSON("기준 뼈", "bone_name", _boneName);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_FLOAT_JSON("충돌 반지름", "collider_radius", _colliderRadius, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("수명(초)", "lifetime", _lifetime, 0.05f, 10.f);
    PROPERTY_BOOL_JSON("Owner Forward 사용", "use_owner_forward", _useOwnerForward);
    PROPERTY_BOOL_JSON("Override Hit Reaction", "use_hit_reaction_override", _useHitReactionOverride);
    PROPERTY_ENUM_JSON("Hit Reaction Type", "override_hit_reaction_type", _overrideHitReactionType, EHitReactionType);
    PROPERTY_BOOL_JSON("Override Launch", "use_launch_override", _useLaunchOverride);
    PROPERTY_FLOAT_JSON("Launch Power", "override_launch_power", _overrideLaunchPower, 0.f, 500.f);
    PROPERTY_FLOAT_JSON("Launch Up", "override_launch_up", _overrideLaunchUp, -50.f, 50.f);
    PROPERTY_STRING_JSON("레이어 태그", "layer_tag", _layerTag);

    return true;
}


void AN_SpawnAttack::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    if (!context.owner)
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    if (!ownerTransform)
        return;

    const Matrix spawnBasisMatrix = Calculate_SpawnBasisMatrix(context, ownerTransform, _boneName);

    SkillObject::FSkillObjectDesc desc{};
    desc.ownerObject = context.owner->GetSharedPtr<GameObject>();
    desc.collisionPreset = _collisionPreset;
    desc.colliderRadius = _colliderRadius;
    desc.lifetime = _lifetime;
    desc.spawnPosition = Calculate_WorldSpawnPosition(spawnBasisMatrix, _localOffset);
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;
    desc.useHitReactionOverride = _useHitReactionOverride;
    desc.hitReactionType = _overrideHitReactionType;
    desc.useLaunchOverride = _useLaunchOverride;
    desc.launchPower = _overrideLaunchPower;
    desc.launchUp = _overrideLaunchUp;

    if (_useOwnerForward)
        desc.direction = ownerTransform->Get_WorldForward();
    else
        desc.direction = Calculate_ForwardFromBasis(spawnBasisMatrix);

    const wstring layerTag = Utils::ToWString(_layerTag);

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        layerTag,
        &desc);

    if (!spawned)
    {
        LOG_WARN("AN_SpawnAttack : 충돌체 소환 실패. type={}, layer={}",
            static_cast<int32>(_spawnObjectType), _layerTag);
        return;
    }

    auto skill = dynamic_pointer_cast<SkillObject>(spawned);
    if (skill)
        skill->Set_Owner(context.owner->GetSharedPtr<GameObject>());
}

Matrix AN_SpawnAttack::Calculate_SpawnBasisMatrix(const FAnimNotifyContext& context, Shared<Transform> transform, const string& boneName)
{
    if (!transform)
        return Matrix::Identity;

    Matrix basisMatrix = transform->Get_WorldMatrix();

    if (boneName.empty() || !context.model)
        return basisMatrix;

    const Matrix* boneMatrix = context.model->Get_SocketBoneMatrixPtr(boneName);
    if (!boneMatrix)
        return basisMatrix;

    return (*boneMatrix) * transform->Get_WorldMatrix();
}

Vec3 AN_SpawnAttack::Calculate_WorldSpawnPosition(const Matrix& basisMatrix, const Vec3& localOffset)
{
    const Vec3 worldPos = basisMatrix.Translation();

    Vec3 right = Vec3::TransformNormal(Vec3(1.f, 0.f, 0.f), basisMatrix);
    Vec3 up = Vec3::TransformNormal(Vec3(0.f, 1.f, 0.f), basisMatrix);
    Vec3 forward = Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), basisMatrix);

    right.Normalize();
    up.Normalize();
    forward.Normalize();

    return worldPos
        + right * localOffset.x
        + up * localOffset.y
        + forward * localOffset.z;
}

Vec3 AN_SpawnAttack::Calculate_ForwardFromBasis(const Matrix& basisMatrix)
{
    Vec3 forward = Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), basisMatrix);
    forward.Normalize();

    return forward;
}
