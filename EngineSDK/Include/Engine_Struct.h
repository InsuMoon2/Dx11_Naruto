#ifndef Engine_Struct_h__
#define Engine_Struct_h__

#include "Engine_Typedef.h"

namespace Engine
{
	typedef struct tagEngineDesc
    {
        HWND        hWnd;
        WINMODE     winMode;
        uint32      viewportWidth;
        uint32      viewportHeight;

    } ENGINE_DESC;
}


#endif // Engine_Struct_h__
