#ifndef Engine_Enum_h__
#define Engine_Enum_h__

namespace Engine
{
	enum class EWinMode { Full, Win };

    enum class EObjectType { GameObject, Component };

    enum class ERenderGroup { Priority, NonBlend, Blend, UI, END };

}
#endif // Engine_Enum_h__
