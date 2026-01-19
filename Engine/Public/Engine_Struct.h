#ifndef Engine_Struct_h__
#define Engine_Struct_h__

#include "Engine_Typedef.h"

namespace Engine
{
	typedef struct tagEngineDesc
    {
        HWND        hWnd;
        WINMODE     windMode;
        _uint       viewportWidth;
        _uint       viewportHeight;

    } ENGINE_DESC;
}


#endif // Engine_Struct_h__
