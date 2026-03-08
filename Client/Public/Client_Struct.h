#pragma once

namespace Client
{
    struct FSkillData
    {
        uint32  skillID = 0;
        wstring skillName = L"";
        wstring IconTexturePath = L"";
        float   coolDown = 0.f;

        // TODO : 데미지 등 추가 예정
    };

}

