#include "pch.h"
#include "AN_BodyCollisionPreset_Change.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "Collider.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_BodyCollisionPreset_Change)
IMPLEMENT_REFLECTION(AN_BodyCollisionPreset_Change)

bool AN_BodyCollisionPreset_Change::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_BodyCollisionPreset_Change";
    info.properties.clear();

    PROPERTY_ENUM("충돌 프리셋", _collisionPreset, Collision_Preset);

    return true;
}

void AN_BodyCollisionPreset_Change::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto bodyCollider = context.owner->Get_Component<Collider>();
    if (!bodyCollider)
        return;

    bodyCollider->Set_CollisionPreset(_collisionPreset);
}

NS_END
