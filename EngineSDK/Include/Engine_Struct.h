#pragma once

namespace Engine
{
    class GameObject;

	typedef struct tagEngineDesc
    {
        HWND        hWnd;
        EWinMode    winMode;

        uint32      numLevels;
        uint32      viewportWidth;
        uint32      viewportHeight;

        uint32      uiReferenceWidth = 1920;
        uint32      uiReferenceHeight = 1080;

    } ENGINE_DESC;

    typedef struct tagEditorDesc
    {
        HWND        hWnd;
        EWinMode    winMode;

        uint32      viewportWidth;
        uint32      viewportHeight;

    } EDITOR_DESC;


    struct FBlackboardKeyInfo
    {
        string name;
        EBlackboardValueType type;
    };

    struct FLightDesc
    {
        ELightType  type;

        Vec4        direction;

        Vec4        position;
        float       range;

        Color       diffuse;
        Color       ambient;
        Color       specular;

        bool        castShadow = false;
        uint32      shadowMapSize = 2048;
        Vec3        shadowCenter = Vec3::Zero;
        float       shadowOrthoWidth = 220.f;
        float       shadowOrthoHeight = 220.f;
        // 수업코드처럼 shadow 전용 perspective 카메라를 직접 쓸지 결정하는 스위치다.
        bool        useShadowCamera = false;
        // shadow 전용 카메라의 eye 위치다. useShadowCamera가 true일 때 사용한다.
        Vec3        shadowEye = Vec3(0.f, 10.f, -7.f);
        // shadow 전용 카메라가 바라보는 target 위치다. useShadowCamera가 true일 때 사용한다.
        Vec3        shadowTarget = Vec3::Zero;
        // shadow 전용 카메라의 수직 시야각(radian)이다. useShadowCamera가 true일 때 사용한다.
        float       shadowFovY = 2.0943951f;
        // shadow 전용 카메라의 종횡비다. 0 이하이면 기존 width/height로 다시 유도한다.
        float       shadowAspect = 1.f;
        float       shadowNear = 1.f;
        float       shadowFar = 450.f;
        float       shadowBias = 0.0015f;
        float       shadowStrength = 0.65f;
        float       shadowSoftness = 1.5f;

    };

    struct FAssetMeta
    {
        string  guid;
        string  type;           // prefab, texture, behavior 등등

        wstring relativePath;
        wstring fullPath;

        // StaticMesh or SkeletalMesh
        string  modelType;
    };

    struct FKeyFrame
    {
        float   time = 0.f;

        Vec3    scale = Vec3(1.f, 1.f, 1.f);
        Quat    rotation = Quat::Identity;
        Vec3    translation = Vec3::Zero;
    };

    struct FAnimationClipSetting
    {
        string  animationName = "";
        bool    loop = true;
        float   playRate = 1.f;
    };

    struct FAnimationLocalPose
    {
        Vec3    scale = Vec3::One;
        Quat    rotation = Quat::Identity;
        Vec3    translation = Vec3::Zero;

        // 해당 Bone Channel이 현재 애니메이션에 실제로 존재하는지 확인용
        bool    valid = false;
    };

    // 현재 재생중인 Clip의 runtime 상태
    struct FPlayingClipState
    {
        int32   animIndex = -1;
        bool    loop = false;
        float   playRate = 1.f;
        float   trackPosition = 0.f;

        bool Is_Valid() const { return animIndex >= 0; }
    };

    // 한 전환에 대한 runtime blend 상태
    struct FAnimationBlendState
    {
        bool    active = false;
        float   duration = 0.15f;
        float   elapsed = 0.f;

        // 전환 대상 clip
        FPlayingClipState next;

        // 시작 pose와 목표 pose 따로 세팅
        vector<FAnimationLocalPose> fromPose;
        vector<FAnimationLocalPose> toPose;

    };

    struct FDamageEvent
    {
        float   damage = 0.f;
        Shared<GameObject> damageCauser = nullptr; // 때린 놈. 플레이어 or 몬스터

        Vec3    damageDir = Vec3::Zero; // 데미지가 들어가는 방향
        bool    hasCustomDir = false;

        float   launchPower = 0.f;
        float   launchUp = 0.f;
        int32   hitSound = 0;       // 이건 추후에 타격 사운드

        EHitReactionType hitReactionType = EHitReactionType::Default;
        uint32 hitReactionSerial = 0;

        string hitAnimStateOverride = ""; // 특정 공격/스킬이 맞은 대상에게 강제로 재생시키고 싶은 피격 애니메이션 상태명이다.

        bool forceHitRestart = false;
    };

    struct FTrailPoint
    {
        Vec3 topPos;
        Vec3 bottomPos;
    };


}
