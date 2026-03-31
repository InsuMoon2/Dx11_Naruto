#pragma once

#include "Engine_Typedef.h"

namespace Engine
{
	enum class EWinMode { Full, Win };

    enum class EObjectType { GameObject, Component };

    enum class ERenderGroup { BackgroundUI, Priority, NonBlend, Blend, UI, END };

    enum class EGameState { Edit, Play, Pause, END };

    enum class EEventType { Create_Object, Delete_Object, END };

    enum class EBTNodeResult { NotExecuted, Succeeded, Failed, InProgress, Aborted };

    enum class EBlackboardValueType { Int, Float, Bool, Vector3, String, END };
 
    enum class ETransformState { View, Proj, END };

    enum class ELightType { Directional, Point, END };

    enum class EUILayer
    {
        HUD,        /* 항상 표시되는 것들 (체력바, 스킬) */
        Navigation, /* 퀘스트 화살표, 정보 전달 용 */
        Popup,      /* 인벤토리, 상점, 일시정지 */
        System,     /* 알림 */
        Overlay,    /* 페이드 인/아웃, 로딩 */
        END
    };

    enum class EMaterialTextureSlot : uint32
    {
        BaseColor = 0,
        Normal,
        Specular,
        Emissive,
        AmbientOcclusion,
        Metalness,
        Roughness,

        END
    };

    enum class ESoundChannel : uint32
    {
        BGM = 0,
        Effect,
        UI,
        Player,
        Monster,
        Boss,
        Ambient,
        Voice,
        System,
        END
    };

    static constexpr uint32 MATERIAL_TEXTURE_SLOT_COUNT =
        static_cast<uint32>(EMaterialTextureSlot::END);

    enum class EMeshVertexType { StaticMesh, SkeletalMesh, END };

    enum class EAnimPhase { Start, Loop, End };

    // 충돌 종류
    enum class EShape { AABB, OBB, Sphere, Capsule, END };

    // 충돌 채널
    enum class Collision_Channel : uint32
    {
        Player          = (1 << 0),   
        Player_Body     = (1 << 1),
        Player_Attack   = (1 << 2),
        Monster         = (1 << 3),     
        Monster_Body    = (1 << 4),
        Monster_Attack  = (1 << 5),
        Weapon          = (1 << 6),
        Projectile      = (1 << 7),
        Item            = (1 << 8),     
        Trigger         = (1 << 9),   
        Enviroment     = (1 << 10),


        CHANNEL_ALL = 0xFFFFFFFF,   // 모든 충돌 OK
        CHANNEL_NONE = 0x00000000,  // 모든 충돌 ㄴㄴ
    };

    // 충돌 프리셋
    enum class Collision_Preset
    {
        Custom,     // 직접 세팅

        Player, Player_Attack,
        Monster, Monster_Attack,

        Weapon,
        Projectile, // 투사체

        Item,
        Trigger,
        Enviroment,

        END
    };

}

