#include "pch.h"
#include "AN_StopParticle.h"
#include "AnimNotify_Factory.h"
#include "AttachedEffectObject.h"
#include "GameObject.h"

REGISTER_ANIM_NOTIFY(AN_StopParticle)
IMPLEMENT_REFLECTION(AN_StopParticle)

bool AN_StopParticle::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_StopParticle";
    info.properties.clear();

    PROPERTY_STRING_JSON("이펙트 이름", "effect_name", _effectAssetName);
    PROPERTY_BOOL_JSON("Owner 이펙트 전부 정지", "stop_all_from_owner", _stopAllFromOwner);

    return true;
}

void AN_StopParticle::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto objects = GAME->Get_GameObjects(GAME->Current_Level());

    for (auto& obj : objects)
    {
        auto attachedEffect = dynamic_pointer_cast<Client::AttachedEffectObject>(obj);
        if (!attachedEffect)
            continue;

        auto effectOwner = attachedEffect->Get_Owner();
        if (!effectOwner || effectOwner != context.owner->GetSharedPtr<GameObject>())
            continue;

        if (_stopAllFromOwner)
        {
            attachedEffect->Stop_AttachedEffect();
            continue;
        }

        if (_effectAssetName.empty())
        {
            attachedEffect->Stop_AttachedEffect();
            continue;
        }

        if (attachedEffect->Get_EffectAssetName() == _effectAssetName)
        {
            attachedEffect->Stop_AttachedEffect();
        }
    }
}
