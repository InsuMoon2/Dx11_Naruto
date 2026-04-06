#include "pch.h"
#include "ANS_AttachSkill.h"
#include "AnimNotify_Factory.h"
#include "SkillObject_Projectile.h"
#include "Model.h"
#include "GameObject.h"
#include "SkillComponent.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_AttachSkill);
IMPLEMENT_REFLECTION(ANS_AttachSkill);

bool ANS_AttachSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_AttachSkill";
    info.properties.clear();

    PROPERTY_STRING_JSON("장착 뼈", "bone_name", _boneName);
    PROPERTY_ENUM_JSON("스킬 타입", "spawn_type", _spawnObjectType, Protocol::OBJECT_TYPE);

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

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        TEXT("Layer_Skill"));

    auto skill = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    CHECK_NULL(skill);

    skill->Set_Owner(context.owner->GetSharedPtr<GameObject>());

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (skillCom)
    {
        skillCom->Set_PendingSkill(_spawnObjectType, skill);
    }
}

void ANS_AttachSkill::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    CHECK_NULL(skillCom);

    auto skill = skillCom->Get_PendingSkill(_spawnObjectType).lock();
    if (!skill || skill->Is_Destroy() || skill->IsLaunched()) return;

    const Matrix* boneMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (!boneMatrix) return;

    Matrix boneWorld = (*boneMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();
    skill->Sync_AttachedTransform(boneWorld);
}

void ANS_AttachSkill::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    // 발사 안 하고 구간이 끝난 경우 SkillComponent가 정리
    auto skillComp = context.owner->Get_Component<SkillComponent>();
    if (skillComp)
        skillComp->Clear_PendingSkill(_spawnObjectType);
}
