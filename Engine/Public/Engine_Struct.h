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
        // ?섏뾽肄붾뱶泥섎읆 shadow ?꾩슜 perspective 移대찓?쇰? 吏곸젒 ?몄? 寃곗젙?섎뒗 ?ㅼ쐞移섎떎.
        bool        useShadowCamera = false;
        // shadow ?꾩슜 移대찓?쇱쓽 eye ?꾩튂?? useShadowCamera媛 true?????ъ슜?쒕떎.
        Vec3        shadowEye = Vec3(0.f, 10.f, -7.f);
        // shadow ?꾩슜 移대찓?쇨? 諛붾씪蹂대뒗 target ?꾩튂?? useShadowCamera媛 true?????ъ슜?쒕떎.
        Vec3        shadowTarget = Vec3::Zero;
        // shadow ?꾩슜 移대찓?쇱쓽 ?섏쭅 ?쒖빞媛?radian)?대떎. useShadowCamera媛 true?????ъ슜?쒕떎.
        float       shadowFovY = 2.0943951f;
        // shadow ?꾩슜 移대찓?쇱쓽 醫낇슒鍮꾨떎. 0 ?댄븯?대㈃ 湲곗〈 width/height濡??ㅼ떆 ?좊룄?쒕떎.
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
        string  type;           // prefab, texture, behavior ?깅벑

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

        // ?대떦 Bone Channel???꾩옱 ?좊땲硫붿씠?섏뿉 ?ㅼ젣濡?議댁옱?섎뒗吏 ?뺤씤??
        bool    valid = false;
    };

    // ?꾩옱 ?ъ깮以묒씤 Clip??runtime ?곹깭
    struct FPlayingClipState
    {
        int32   animIndex = -1;
        bool    loop = false;
        float   playRate = 1.f;
        float   trackPosition = 0.f;

        bool Is_Valid() const { return animIndex >= 0; }
    };

    // ???꾪솚?????runtime blend ?곹깭
    struct FAnimationBlendState
    {
        bool    active = false;
        float   duration = 0.15f;
        float   elapsed = 0.f;

        // ?꾪솚 ???clip
        FPlayingClipState next;

        // ?쒖옉 pose? 紐⑺몴 pose ?곕줈 ?명똿
        vector<FAnimationLocalPose> fromPose;
        vector<FAnimationLocalPose> toPose;

    };

    struct FDamageEvent
    {
        float   damage = 0.f;
        Shared<GameObject> damageCauser = nullptr; // ?뚮┛ ?? ?뚮젅?댁뼱 or 紐ъ뒪??

        Vec3    damageDir = Vec3::Zero; // ?곕?吏媛 ?ㅼ뼱媛??諛⑺뼢
        bool    hasCustomDir = false;

        float   launchPower = 0.f;
        float   launchUp = 0.f;
        int32   hitSound = 0;       // ?닿굔 異뷀썑???寃??ъ슫??
        string  hitSoundFile = "";  // ComboProfile/스킬에서 직접 넘긴 피격 사운드 파일명이다.
        uint32  damageSourceType = 0; // ?ㅼ쭏?곸쑝濡??곕?吏瑜?以 媛앹껜(SkillObject ???????

        EHitReactionType hitReactionType = EHitReactionType::Default;
        uint32 hitReactionSerial = 0;

        string hitAnimStateOverride = ""; // ?뱀젙 怨듦꺽/?ㅽ궗??留욎? ??곸뿉寃?媛뺤젣濡??ъ깮?쒗궎怨??띠? ?쇨꺽 ?좊땲硫붿씠???곹깭紐낆씠??

        bool forceHitRestart = false;
    };

    struct FTrailPoint
    {
        Vec3 topPos;
        Vec3 bottomPos;
    };


}
