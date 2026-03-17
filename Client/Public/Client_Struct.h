#pragma once

#include "Client_Enum.h"

NS_BEGIN(Engine)
struct FAnimationClipSetting;
NS_END

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

    struct FStateAnimationDesc
    {
        EStateAnimationMode mode = EStateAnimationMode::Single;

        // 싱글용
        FAnimationClipSetting single;

        // 시퀀스용
        FAnimationClipSetting start;
        FAnimationClipSetting loop;
        FAnimationClipSetting end;

        float	playRate = 1.f;

        // 루프 애니메이션이 있으면 시퀀스로 판단
        bool Has_Sequence() const
        {
            return !loop.animationName.empty();
        }
    };

}

