#pragma once

#include "Client_Enum.h"

NS_BEGIN(Engine)
struct FAnimationClipSetting;
NS_END

namespace Client
{
    struct FSkillData
    {
        uint32   skill_Id = 0;
        wstring  skillName = L"";
        float    coolDown = 0.f;

        string   animStateName = "";
        float    loopDurationSec = 0.f;

        bool     isHoldSkill = false;
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

        uint32 skillIconSrvIndex = 0;
    };

    struct FDirectionClipDesc
    {
        FAnimationClipSetting forward;
        FAnimationClipSetting backward;
        FAnimationClipSetting right;
        FAnimationClipSetting left;

        // 뭐라도 있나
        bool Has_Any() const
        {
            return !forward.animationName.empty()
                || !backward.animationName.empty()
                || !left.animationName.empty()
                || !right.animationName.empty();
        }

        const FAnimationClipSetting* Find(EMoveInputDirection dir) const
        {
            switch (dir)
            {
            case EMoveInputDirection::Forward:
                if (!forward.animationName.empty()) return &forward;
                break;

            case EMoveInputDirection::Backward:
                if (!backward.animationName.empty()) return &backward;
                break;

            case EMoveInputDirection::Left:
                if (!left.animationName.empty()) return &left;
                break;

            case EMoveInputDirection::Right:
                if (!right.animationName.empty()) return &right;
                break;
            }

            if (!forward.animationName.empty())
                return &forward;

            return nullptr;
        }
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

        // 방향성 있는 싱글
        FDirectionClipDesc    directional;

        bool Has_Sequence() const
        {
            return !start.animationName.empty()
                || !loop.animationName.empty()
                || !end.animationName.empty();
        }
    };

   

}

