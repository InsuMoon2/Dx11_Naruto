#include "pch.h"
#include "AN_PlayCinematic.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "Transform.h"
#include "Utils.h"

REGISTER_ANIM_NOTIFY(AN_PlayCinematic)
IMPLEMENT_REFLECTION(AN_PlayCinematic)

bool AN_PlayCinematic::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_PlayCinematic";
    info.properties.clear();

    PROPERTY_STRING("시퀀스 이름", _sequenceName);
    PROPERTY_BOOL("전역 입력 차단", _blockGameInput);

    return true;
}

string AN_PlayCinematic::Get_TypeName() const
{
    return "AN_PlayCinematic";
}

void AN_PlayCinematic::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    if (!context.owner)
        return;

    if (_sequenceName.empty())
        return;

    auto ownerTransform = context.owner->Get_Transform();
    if (!ownerTransform)
        return;

    const bool played = GAME->Play_Cinematic(Utils::ToWString(_sequenceName), ownerTransform, _blockGameInput);

    if (!played)
    {
        LOG_WARN("AN_PlayCinematic failed. sequence='{}'", _sequenceName);
    }
}

json AN_PlayCinematic::Serialize_Payload() const
{
    json j;
    j["sequence_name"] = _sequenceName;
    j["block_game_input"] = _blockGameInput;
    return j;
}

void AN_PlayCinematic::Deserialize_Payload(const json& payload)
{
    _sequenceName = payload.value("sequence_name", string{});
    _blockGameInput = payload.value("block_game_input", false);
}
