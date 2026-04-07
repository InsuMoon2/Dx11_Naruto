#include "pch.h"
#include "AN_SpawnAttack.h"
#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject.h"
#include "Client_Defines.h"

REGISTER_ANIM_NOTIFY(AN_SpawnAttack)
IMPLEMENT_REFLECTION(AN_SpawnAttack)

bool AN_SpawnAttack::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnAttack";
    info.properties.clear();

    PROPERTY_ENUM_JSON("스킬 오브젝트 타입", "spawn_object_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_FLOAT_JSON("충돌 반지름", "collider_radius", _colliderRadius, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("수명(초)", "lifetime", _lifetime, 0.05f, 10.f);
    PROPERTY_BOOL_JSON("Forward 사용", "use_owner_forward", _useOwnerForward);
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

    SkillObject::FSkillObjectDesc desc{};
    desc.collisionPreset = _collisionPreset;
    desc.colliderRadius = _colliderRadius;
    desc.lifetime = _lifetime;
    desc.spawnPosition = Calculate_WorldSpawnPosition(ownerTransform, _localOffset);
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;

    if (_useOwnerForward)
        desc.direction = ownerTransform->Get_WorldForward();
    else
        desc.direction = Vec3::Forward;

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

Vec3 AN_SpawnAttack::Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset)
{
    const Vec3 worldPos = transform->Get_WorldPosition();
    const Vec3 right = transform->Get_WorldRight();
    const Vec3 up = transform->Get_WorldUp();
    const Vec3 forward = transform->Get_WorldForward();

    return worldPos
        + right * localOffset.x
        + up * localOffset.y
        + forward * localOffset.z;
}
