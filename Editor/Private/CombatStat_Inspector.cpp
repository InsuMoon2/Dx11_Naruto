#include "pch.h"
#include "CombatStat_Inspector.h"
#include "Component.h"

void CombatStat_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    json data = component->To_Json();

    if (!Draw_Header("Combat Stat"))
        return;

    // JSON에서 값 읽기
    float hp        = data.value("hp", 0.f);
    float maxHp     = data.value("maxHp", 0.f);
    float mp        = data.value("mp", 0.f);
    float maxMp     = data.value("maxMp", 0.f);
    float attack    = data.value("attack", 0.f);
    float defense   = data.value("defense", 0.f);
    float speed     = data.value("speed", 0.f);

    Draw_ReadOnly("Hp : ", hp);
    //Draw_ProgressBar(hp, maxHp);

    Draw_ReadOnly("Mp : ", mp);
    //Draw_ProgressBar(mp, maxMp);

    if (Draw_Float("Attack : ", attack))
    {
        data["attack"] = attack;
    }

    if (Draw_Float("Defense : ", defense))
    {
        data["defense"] = defense;
    }

    if (Draw_Float("Speed : ", speed))
    {
        data["speed"] = speed;
    }

}
