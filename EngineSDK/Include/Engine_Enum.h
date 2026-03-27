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

}

