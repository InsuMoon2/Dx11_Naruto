#pragma once

namespace Engine
{
	enum class EWinMode { Full, Win };

    enum class EObjectType { GameObject, Component };

    enum class ERenderGroup { Priority, NonBlend, Blend, UI, END };

    enum class EGameState { Edit, Play, Pause, END };

    enum class EEventType { Create_Object, Delete_Object, END };

    enum class EBTNodeResult { NotExecuted, Succeeded, Failed, InProgress, Aborted };

    enum class EBlackboardValueType { Int, Float, Bool, Vector3, END };
 
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

    enum class EModelType { StaticMesh, SkeletalMesh, END };
}

