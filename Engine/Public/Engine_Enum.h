#pragma once

namespace Engine
{
	enum class EWinMode { Full, Win };

    enum class EObjectType { GameObject, Component };

    enum class ERenderGroup { Priority, NonBlend, Blend, UI, END };

    enum class EGameState { Edit, Play, Pause, END };

    enum class EEventType { Create_Object, Delete_Object, END };

    enum class EBTNodeResult { Succeeded, Failed, InProgress, Aborted };

    enum class EBlackboardValueType { Int, Float, Bool, Vector3, END };
 
    enum class EUITransformState { View, Proj, END };
}

