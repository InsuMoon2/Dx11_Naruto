#pragma once

#include "AnimNotify_Inspector.h"

NS_BEGIN(Editor)

class AN_SpawnSkill_Inspector final : public AnimNotify_Inspector
{
public:
    void Draw_Inspector(Shared<AnimNotify> notify) override;

private:
    static vector<pair<Protocol::OBJECT_TYPE, string>> Build_SpawnableItems();
};

NS_END
