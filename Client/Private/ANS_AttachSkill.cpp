#include "pch.h"
#include "ANS_AttachSkill.h"
#include "AnimNotify_Factory.h"
#include "SkillObject_Projectile.h"
#include "Model.h"
#include "GameObject.h"
#include "SkillComponent.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_AttachSkill);
IMPLEMENT_REFLECTION(ANS_AttachSkill);

static umap<GameObject*, umap<Protocol::OBJECT_TYPE, Weak<SkillObject_Projectile>>> s_attachSkillFallbackPendingSkills;

bool ANS_AttachSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_AttachSkill";
    info.properties.clear();

    PROPERTY_STRING_JSON("장착 뼈", "bone_name", _boneName);
    PROPERTY_ENUM_JSON("스킬 타입", "spawn_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);

    PROPERTY_VEC3_JSON("부착 위치 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_VEC3_JSON("부착 회전 각도(Pitch,Yaw,Roll)", "local_rotation", _localRotation, 0.1f);

    return true;
}

string ANS_AttachSkill::Get_TypeName() const
{
    return "ANS_AttachSkill";
}

void ANS_AttachSkill::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    if (!ownerTransform)
        return;

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.collisionPreset = _collisionPreset;
    desc.startAttached = true;
    desc.spawnPosition = ownerTransform->Get_WorldPosition();
    desc.direction = ownerTransform->Get_WorldForward();

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        TEXT("Layer_Skill"), &desc);

    auto skill = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    CHECK_NULL(skill);

    skill->Set_Owner(context.owner->GetSharedPtr<GameObject>());
    _attachedSkill = skill;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (skillCom)
    {
        skillCom->Set_PendingSkill(_spawnObjectType, skill);
        Clear_FallbackPendingSkill(context.owner, _spawnObjectType, false);
        return;
    }

    Set_FallbackPendingSkill(context.owner, _spawnObjectType, skill);
}

void ANS_AttachSkill::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    Shared<SkillObject_Projectile> skill = nullptr;

    if (skillCom)
        skill = skillCom->Get_PendingSkill(_spawnObjectType).lock();

    if (!skill)
        skill = Get_FallbackPendingSkill(context.owner, _spawnObjectType).lock();

    if (!skill || skill->Is_Destroy() || skill->IsLaunched()) return;
    if (!context.model) return;

    const Matrix* boneMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (!boneMatrix) return;

    Matrix offsetRot = Matrix::CreateRotationX(XMConvertToRadians(_localRotation.x)) *
                   Matrix::CreateRotationY(XMConvertToRadians(_localRotation.y)) *
                   Matrix::CreateRotationZ(XMConvertToRadians(_localRotation.z));

    Matrix offsetTrans = Matrix::CreateTranslation(_localOffset);
    Matrix boneWorld = offsetRot * offsetTrans * (*boneMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();

    skill->Sync_AttachedTransform(boneWorld);
}

void ANS_AttachSkill::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    // 발사 안 하고 구간이 끝난 경우 SkillComponent가 정리
    auto skillComp = context.owner->Get_Component<SkillComponent>();
    if (skillComp)
        skillComp->Clear_PendingSkill(_spawnObjectType);
    else
        Clear_FallbackPendingSkill(context.owner, _spawnObjectType, true);

    _attachedSkill.reset();
}

void ANS_AttachSkill::Set_FallbackPendingSkill(GameObject* owner, Protocol::OBJECT_TYPE type, Shared<SkillObject_Projectile> skill)
{
    if (!owner || !skill)
        return;

    s_attachSkillFallbackPendingSkills[owner][type] = skill;
}

Weak<SkillObject_Projectile> ANS_AttachSkill::Get_FallbackPendingSkill(GameObject* owner, Protocol::OBJECT_TYPE type)
{
    if (!owner)
        return {};

    auto ownerIter = s_attachSkillFallbackPendingSkills.find(owner);
    if (ownerIter == s_attachSkillFallbackPendingSkills.end())
        return {};

    auto skillIter = ownerIter->second.find(type);
    if (skillIter == ownerIter->second.end())
        return {};

    return skillIter->second;
}

void ANS_AttachSkill::Clear_FallbackPendingSkill(GameObject* owner, Protocol::OBJECT_TYPE type, bool destroyIfAttached)
{
    if (!owner)
        return;

    auto ownerIter = s_attachSkillFallbackPendingSkills.find(owner);
    if (ownerIter == s_attachSkillFallbackPendingSkills.end())
        return;

    auto skillIter = ownerIter->second.find(type);
    if (skillIter == ownerIter->second.end())
        return;

    auto skill = skillIter->second.lock();
    if (destroyIfAttached && skill && !skill->IsLaunched())
        skill->Set_Destroy(true);

    ownerIter->second.erase(skillIter);

    if (ownerIter->second.empty())
        s_attachSkillFallbackPendingSkills.erase(ownerIter);
}
