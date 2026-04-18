#pragma once

#include "Client_Enum.h"
#include "Engine_Enum.h"

NS_BEGIN(Engine)
struct FAnimationClipSetting;
NS_END

namespace Client
{
    struct FSkillData
    {
        uint32  skill_Id = 0;
        wstring skillName = L"";
        float   coolDown = 0.f;

        string  animStateName = "";

        // 차징
        float   loopDurationSec = 0.f;
        bool    isHoldSkill = false;

        // 대쉬 루프
        bool    hasDashPhase = false;
        float   dashSpeed = 15.f;

        float   maxDashDistance = 20.f;     // 이 거리 넘으면 강제로 Attack End로 세팅
        float   targetStopDistance = 1.5f;  // 타겟 근처로 가면 멈출 값

        // true면 락온 타겟에 가까워져도 조기 종료 ㄴㄴ
        bool    ignoreTargetStop = false;

        // 대쉬 루프 이후 애니메이션 -> 나선환, 치도리 말고 또 있나 쓸 데가
        string  attackEndAnimStateName = "";

        // 공중 스킬
        string  airAnimStateName = "";
        bool    airGravityOff = false;

        string  airLandedAnimStateName = "";
        string  airAttackEndAnimStateName = "";

        ESkillCategory skillCategory = ESkillCategory::Main;
        int32 uiSlotIndex = -1;
        uint32 uiIconSrvIndex = 0;
    };

    struct FComboEntry
    {
        string  animStateKey;
        float   damageMultiplier = 1.f;
        bool    canCancel = true;

        float   launchPower = 0.f;
        float   launchUp = 0.f;
        int32   hitSound = 0;
    };

    struct FComboProfile
    {
        string              profileName;

        EAttackProfileType  profileType = EAttackProfileType::Hand_Ground;
        EWeaponType         weaponType = EWeaponType::Hand;
        bool                isAerial = false;
        int32               maxCombo = 4;
        vector<FComboEntry> combos;
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

        // 뭐라도 있나 확인
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

