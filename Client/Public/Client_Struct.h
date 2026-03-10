#pragma once

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

}

