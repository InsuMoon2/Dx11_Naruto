#ifndef Engine_Struct_h__
#define Engine_Struct_h__

#include "Engine_Typedef.h"

namespace Engine
{
	typedef struct tagEngineDesc
    {
        HWND        hWnd;
        EWinMode     winMode;

        uint32      numLevels;
        uint32      viewportWidth;
        uint32      viewportHeight;

    } ENGINE_DESC;

    typedef struct tagEditorDesc
    {
        HWND        hWnd;
        EWinMode     winMode;

        uint32      viewportWidth;
        uint32      viewportHeight;

    } EDITOR_DESC;
}


#endif // Engine_Struct_h__
