#pragma once

#include <Windows.h>
#include "GameInstance.h"
#include "EditorInstance.h"

extern HWND g_hWnd;
extern HINSTANCE g_hInst;


namespace Client
{
    extern unsigned int		g_winSizeX;
    extern unsigned int		g_winSizeY;
    extern bool             g_enableEditor;

    enum class ELevelType { Static, Loading, Logo, GamePlay, END };
}

using namespace Client;
