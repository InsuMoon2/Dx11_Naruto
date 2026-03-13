#pragma once

#include "Client_Enum.h"

namespace Client
{
    struct FSkillData
    {
        uint32  skill_Id = 0;
        wstring skillName = L"";
        uint32   srvIndex = 0;
        float    coolDown = 0.f;
        int      manaCost = 0;
    };

    struct FLoadJob
    {
        ELoadJobType type = ELoadJobType::TextureCreate;

        uint32 componentID = 0;
        uint32 levelIndex = 0;
        uint32 prototypeLevelIndex = 0;
        uint32 objectType = 0;

        string idStr;
        string pathStr;
        string extraStr;

        uint32 count = 1;
        bool isSkeletal = false;

        FSkillData skillData{};
    };

}

