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

    PROPERTY_STRING("Target (weapon/hitbox)", _target);
    PROPERTY_BOOL("Right Hand", _useRightHand);
    PROPERTY_BOOL("Left Hand", _useLeftHand);
    PROPERTY_BOOL("Right Foot", _useRightFoot);
    PROPERTY_BOOL("Left Foot", _useLeftFoot);

    return true;
}

string ANS_CollisionEnable::Get_TypeName() const
{
    return "ANS_CollisionEnable";
}

void ANS_CollisionEnable::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
    CHECK_NULL(myPlayer);

    // 새로 공격 시 히트 리스트 초기화
    auto combatStat = myPlayer->Get_Component<CombatStat>();
    if (combatStat)
        combatStat->Begin_AttackSwing();

    if (_target == "weapon")
    {
        auto container = dynamic_cast<ContainerObject*>(context.owner);
        CHECK_NULL(container);

        auto weaponPart = dynamic_pointer_cast<Weapon>(
            container->Get_PartObject(ContainerObject::EPartSlot::Weapon));

        if (weaponPart)
            weaponPart->Set_ColliderActive(true);
    }
    else // hitbox일 때 -> 격투형
    {
        if (_useRightHand) myPlayer->Enable_Hitbox(EHitboxTarget::RightHand);
        if (_useLeftHand)  myPlayer->Enable_Hitbox(EHitboxTarget::LeftHand);
        if (_useRightFoot) myPlayer->Enable_Hitbox(EHitboxTarget::RightFoot);
        if (_useLeftFoot)  myPlayer->Enable_Hitbox(EHitboxTarget::LeftFoot);
    }

}

void ANS_CollisionEnable::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;


}

void ANS_CollisionEnable::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
    if (!myPlayer) return;

    if (_target == "weapon")
    {
        auto container = dynamic_cast<ContainerObject*>(context.owner);
        if (!container) return;
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

json ANS_CollisionEnable::Serialize_Payload() const
{
    json j;
    j["target"] = _target;
    j["right_hand"] = _useRightHand;
    j["left_hand"] = _useLeftHand;
    j["right_foot"] = _useRightFoot;
    j["left_foot"] = _useLeftFoot;

    return j;
}

void ANS_CollisionEnable::Deserialize_Payload(const json& payload)
{
    if (payload.contains("target"))     _target = payload["target"].get<string>();
    if (payload.contains("right_hand")) _useRightHand = payload["right_hand"].get<bool>();
    if (payload.contains("left_hand"))  _useLeftHand = payload["left_hand"].get<bool>();
    if (payload.contains("right_foot")) _useRightFoot = payload["right_foot"].get<bool>();
    if (payload.contains("left_foot"))  _useLeftFoot = payload["left_foot"].get<bool>();

    AnimNotifyState::Deserialize_Payload(payload);
}
