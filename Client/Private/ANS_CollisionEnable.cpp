#include "pch.h"
#include "ANS_CollisionEnable.h"
#include "GameObject.h"
#include "MyPlayer.h"
#include "ContainerObject.h"
#include "Weapon.h"
#include "AnimNotify_Factory.h"
#include "CombatStat.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_CollisionEnable);
IMPLEMENT_REFLECTION(ANS_CollisionEnable);

bool ANS_CollisionEnable::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_CollisionEnable";
    info.properties.clear();

    PROPERTY_STRING_JSON("Target (weapon/hitbox)", "target", _target);

    PROPERTY_BOOL_JSON("Right Hand", "right_hand", _useRightHand);
    PROPERTY_BOOL_JSON("Left Hand", "left_hand", _useLeftHand);
    PROPERTY_BOOL_JSON("Right Foot", "right_foot", _useRightFoot);
    PROPERTY_BOOL_JSON("Left Foot", "left_foot", _useLeftFoot);

    PROPERTY_BOOL_JSON("Override Hit Reaction", "use_hit_reaction_override", _useHitReactionOverride);
    PROPERTY_ENUM_JSON("Hit Reaction Type", "override_hit_reaction_type", _overrideHitReactionType, EHitReactionType);

    PROPERTY_BOOL_JSON("Override Launch", "use_launch_override", _useLaunchOverride);
    PROPERTY_FLOAT_JSON("Launch Power", "override_launch_power", _overrideLaunchPower, 0.f, 50.f);
    PROPERTY_FLOAT_JSON("Launch Up", "override_launch_up", _overrideLaunchUp, -50.f, 50.f);

    return true;
}

string ANS_CollisionEnable::Get_TypeName() const
{
    return "ANS_CollisionEnable";
}

void ANS_CollisionEnable::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
    CHECK_NULL(myPlayer);

    // 현재 타격 구간이 새로 시작될 때 이전 히트 목록과 override를 새 구간 기준으로 초기화한다.
    auto combatStat = myPlayer->Get_Component<CombatStat>();
    if (combatStat)
    {
        combatStat->Begin_AttackSwing();

        CombatStat::FAttackSwingOverride overrideDesc{};
        overrideDesc.useHitReactionOverride = _useHitReactionOverride;
        overrideDesc.hitReactionType = _overrideHitReactionType;
        overrideDesc.useLaunchOverride = _useLaunchOverride;
        overrideDesc.launchPower = _overrideLaunchPower;
        overrideDesc.launchUp = _overrideLaunchUp;

        combatStat->Set_AttackSwingOverride(overrideDesc);
    }

    if (_target == "weapon")
    {
        auto container = dynamic_cast<ContainerObject*>(context.owner);
        CHECK_NULL(container);

        auto weaponPart = dynamic_pointer_cast<Weapon>(
            container->Get_PartObject(ContainerObject::EPartSlot::Weapon));

        if (weaponPart)
            weaponPart->Set_ColliderActive(true);
    }
    else
    {
        if (_useRightHand) myPlayer->Enable_Hitbox(EHitboxTarget::RightHand);
        if (_useLeftHand)  myPlayer->Enable_Hitbox(EHitboxTarget::LeftHand);
        if (_useRightFoot) myPlayer->Enable_Hitbox(EHitboxTarget::RightFoot);
        if (_useLeftFoot)  myPlayer->Enable_Hitbox(EHitboxTarget::LeftFoot);
    }
}

void ANS_CollisionEnable::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;
}

void ANS_CollisionEnable::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
    if (!myPlayer)
        return;

    auto combatStat = myPlayer->Get_Component<CombatStat>();
    if (combatStat)
        combatStat->Clear_AttackSwingOverride();

    if (_target == "weapon")
    {
        auto container = dynamic_cast<ContainerObject*>(context.owner);
        if (!container)
            return;

        auto weaponPart = dynamic_pointer_cast<Weapon>(
            container->Get_PartObject(ContainerObject::EPartSlot::Weapon));

        if (weaponPart)
            weaponPart->Set_ColliderActive(false);
    }
    else
    {
        if (_useRightHand) myPlayer->Disable_Hitbox(EHitboxTarget::RightHand);
        if (_useLeftHand)  myPlayer->Disable_Hitbox(EHitboxTarget::LeftHand);
        if (_useRightFoot) myPlayer->Disable_Hitbox(EHitboxTarget::RightFoot);
        if (_useLeftFoot)  myPlayer->Disable_Hitbox(EHitboxTarget::LeftFoot);
    }
}
