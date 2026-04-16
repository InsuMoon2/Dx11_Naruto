#pragma once

#include <Windows.h>
#include "GameInstance.h"
//#include "EditorInstance.h"

extern HWND g_hWnd;
extern HINSTANCE g_hInst;

namespace Client
{
    extern unsigned int		g_winSizeX;
    extern unsigned int		g_winSizeY;
    extern bool             g_enableEditor;

    enum class ELevelType { Static, Loading, MainTitle, CharacterSetup, Tutorial, GamePlay, Konoha, Prefab, END };
}

using namespace Client;
using Message = google::protobuf::Message;

#include "Client_Enum.h"
#include "Client_Macro.h"
#include "Client_Struct.h"
