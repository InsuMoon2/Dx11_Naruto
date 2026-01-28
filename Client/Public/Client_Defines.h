#pragma once

#include <Windows.h>
#include "GameInstance.h"
#include "EditorInstance.h"

extern HWND g_hWnd;
extern HINSTANCE g_hInst;


namespace Client
{
    const unsigned int		g_winSizeX = { 1600 };
    const unsigned int		g_winSizeY = { 900 };

    enum class LevelType { Static, Loading, Logo, GamePlay, END };
}

using namespace Client;
