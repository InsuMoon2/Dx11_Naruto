#include "pch.h"
#include "AN_SpawnSkill.h"

#include "GameObject_Factory.h"
#include "AnimNotify_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject_Projectile.h"
#include "Client_Defines.h"

REGISTER_ANIM_NOTIFY(AN_SpawnSkill)
IMPLEMENT_REFLECTION(AN_SpawnSkill)

bool AN_SpawnSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnSkill";
    info.properties.clear();

    PROPERTY_INT("스킬 ID", _skill_Id, 0, 9999);
    PROPERTY_VEC3("로컬 오프셋", _localOffset, 0.1f);
    PROPERTY_STRING("레이어 태그", _layerTag);
    PROPERTY_BOOL("Forward 사용", _useOwnerForward);

    return true;
}

string AN_SpawnSkill::Get_TypeName() const
{
    return "AN_SpawnSkill";
}

void AN_SpawnSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    if (!context.owner)
        return;

    if (!GameObject_Factory::Is_ObjectInCategory(_spawnObjectType, "SkillSpawn"))
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    if (!ownerTransform)
        return;

    SkillObject_Projectile::FProjectileSkillDesc desc;
    desc.ownerSkillId = _skill_Id;
    desc.spawnPosition = Calculate_WorldSpawnPosition(ownerTransform, _localOffset);
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;

    if (_useOwnerForward)
        desc.direction = ownerTransform->Get_WorldForward();
    else
        desc.direction = Vec3::Forward;

    const uint32 currentLevel = GAME->Current_Level();
    const wstring layerTag = Utils::ToWString(_layerTag);

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        currentLevel,
        layerTag,
        &desc);

    if (!spawned)
    {
        LOG_WARN("AN_SpawnSkill : failed to spawn. type={}, skillId={}",
            static_cast<int32>(_spawnObjectType), _skill_Id);
        return;
    }

}

json AN_SpawnSkill::Serialize_Payload() const
{
    json j;
    j["spawn_object_type"] = string(magic_enum::enum_name(_spawnObjectType));
    j["skill_id"] = _skill_Id;
    j["local_offset"] = Utils::Vec3_ToJson(_localOffset);
    j["layer_tag"] = _layerTag;
    j["use_owner_forward"] = _useOwnerForward;

    return j;
}

void AN_SpawnSkill::Deserialize_Payload(const json& payload)
{
    const string typeName = payload.value("spawn_object_type", string{});

    auto objectType = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(typeName);
    if (objectType.has_value())
        _spawnObjectType = objectType.value();

    _skill_Id = payload.value("skill_id", 0);
    _localOffset = Utils::Vec3_FromJson(payload.value("local_offset", json::array()), Vec3(0.f, 1.2f, 1.8f));
    _layerTag = payload.value("layer_tag", string("Layer_SkillObject"));
    _useOwnerForward = payload.value("use_owner_forward", true);
}

Vec3 AN_SpawnSkill::Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset)
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
