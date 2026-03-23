#pragma once

#include <Windows.h>
#include "GameInstance.h"
//#include "EditorInstance.h"

#include "../../Client/Public/Client_Defines.h"

extern HWND g_hWnd;
extern HINSTANCE g_hInst;

namespace Client
{
    extern unsigned int		g_winSizeX;
    extern unsigned int		g_winSizeY;
    extern bool             g_enableEditor;
}

using namespace Client;
using Message = google::protobuf::Message;
